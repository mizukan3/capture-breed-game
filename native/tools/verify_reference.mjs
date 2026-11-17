import fs from 'node:fs';
import vm from 'node:vm';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const htmlBytes=fs.readFileSync(path.join(root,'index.html'));
const blob=createHash('sha1').update(Buffer.from('blob '+htmlBytes.length+'\0')).update(htmlBytes).digest('hex');
const provenance=JSON.parse(fs.readFileSync(path.join(root,'native/data/provenance.json'),'utf8'));
assert.equal(blob,provenance.git_blob,'Reference HTML changed: review provenance instead of silently changing expectations.');
const html=htmlBytes.toString('utf8'),names=['data.js','monster-kit.js','monster-combat.js','progression.js','engine.js','view.js','save-store.js'];
let source='const __modules=Object.create(null);\n';
for(const name of names) {
 const start=html.indexOf('__modules["'+name+'"] = (');
 assert(start>=0,'missing module '+name);
 const end=html.indexOf('\n})();',start);assert(end>start,'missing module terminator '+name);
 source+=html.slice(start,end+6)+'\n';
}
source+='globalThis.reference=__modules;';
const sandbox=vm.createContext({});vm.runInContext(source,sandbox,{timeout:10000});
const ref=sandbox.reference,engine=ref['engine.js'],data=ref['data.js'];
const game=new engine.Game(),requests=[],expected=[];
const add=(r,v)=>{requests.push(r);expected.push(JSON.parse(JSON.stringify(v)));};
add({op:'data'},{families:data.FAMILIES,species:data.SPECIES,dungeons:data.DUNGEONS,themes:data.THEMES,items:data.ITEMS,skills:ref['monster-kit.js'].SKILLS});
for(let id=0;id<data.SPECIES.length;id++)for(let level=1;level<=100;level++)add({op:'stats',species:id,level},data.speciesStats(id,level));
for(let level=1;level<=100;level++){
 add({op:'player',level},data.playerBaseStats(level));
 for(const monster of [false,true])add({op:'xp',level,monster},game.xpNeeded(level,monster));
}
for(const attack of [0,1,3,5,11,25,100,300,999])for(const defense of [0,1,3,5,11,25,100,300,999])add({op:'damage',attack,defense},data.normalDamage(attack,defense));
const seeds=[0,1,2,13,42,65535,2147483647,2147483648,4294967295];
for(let i=0;i<48;i++)seeds.push((Math.imul(i+1,2654435761)>>>0));
for(const seed of seeds) {
 game.state.rng=seed;const values=Array.from({length:128},()=>game.random());
 add({op:'rng',seed,count:128},{values,state:game.state.rng});
 for(let floor=1;floor<=6;floor++) {
  Object.assign(game.state,{floor,rng:seed,uid:11,seen:[],roster:[]});game.generateFloor();
  const s=game.state,m=s.map;
  const keys=['uid','species','level','x','z','hp','maxHp','atk','def','boss'];
  add({op:'floor',seed,floor},{size:m.size,tiles:m.tiles,visible:m.visible,visited:m.visited,rooms:m.rooms,stairs:m.stairs,spawn:{x:s.player.x,z:s.player.z},enemies:s.enemies.map(e=>Object.fromEntries(keys.map(k=>[k,e[k]]))),drops:s.drops,rng:s.rng,uid:s.uid,seen:s.seen});
  if(seed===13)for(const [x,z] of [[7,7],[3,3],[10,10]]) {
   const sight=[];for(let zz=0;zz<m.size;zz++)for(let xx=0;xx<m.size;xx++)sight.push(game.lineOfSight(x,z,xx,zz));
   add({op:'sight',seed,floor,x,z},sight);
  }
 }
}
const attackGame=new engine.Game();
for(const seed of seeds)for(const sureHit of [false,true])for(const unguardable of [false,true])for(const frozen of [false,true]) {
 const attacker={maxHp:15,atk:11,def:5,evasion:5,accuracy:90,critical:37,guardRate:0};
 const defender={maxHp:25,atk:5,def:7,evasion:20,accuracy:100,critical:5,guardRate:23};
 const a={x:1,z:1,sureHit:sureHit?1:0},b={x:1,z:2,species:0,hp:99999,freeze:frozen?1:0};
 let kind='hit',critical=false;
 attackGame.state.rng=seed;attackGame.stats=m=>m===a?attacker:defender;
 attackGame.combatEvent=type=>{if(type==='miss'||type==='guard')kind=type;};
 attackGame.event=type=>{if(type==='critical')critical=true;};
 attackGame.log=()=>{};attackGame.directDamage=(_target,n)=>n;
 const damage=attackGame.hit(a,b,'test',{unguardable});
 add({op:'attack',seed,attacker,defender,sureHit,unguardable,frozen},{kind,damage,critical,rng:attackGame.state.rng});
}
for(let species=0;species<5;species++)for(const hp of [0,1,3,4,5,8,17,25])for(const flag of ['none','boss','summoned','decoy','fear']) {
 const e=game.makeEnemy(species,1,{x:1,z:1});Object.assign(e,{hp,[flag]:true});
 add({op:'capture',species,hp,maxHp:e.maxHp,boss:!!e.boss,summoned:!!e.summoned,decoy:!!e.decoy,fear:!!e.fear,traitFear:species===2&&hp>0&&hp<=e.maxHp*.4},game.captureCondition(e));
}
for(const text of ['', 'abc','探索者・ピョン','🧪😀','{"name":"ヤミピョン","hp":25}\n'])add({op:'checksum',text},ref['save-store.js'].saveChecksum(text));
for(const aspect of [.45,1,1.78,2.4])for(const angle of [0,Math.PI/2,Math.PI,Math.PI*1.5])for(const zoom of [0,1]) {
 const r={op:'camera',x:7,z:10,aspect,angle,zoom};add(r,ref['view.js'].cameraRig(r.x,r.z,aspect,0,zoom,angle));
}
const executable=process.argv[2];if(!executable)throw new Error('Pass the built native-probe executable path.');
const run=spawnSync(executable,[],{input:requests.map(JSON.stringify).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024});
assert.equal(run.status,0,run.stderr||String(run.error));
const actual=run.stdout.trim().split(/\r?\n/).map(s=>JSON.parse(s));
assert.equal(actual.length,expected.length,'probe result count');
function equivalent(a,b,where) {
 if(typeof a==='number'&&typeof b==='number')assert(Math.abs(a-b)<=1e-10*Math.max(1,Math.abs(b)),where+': '+a+' != '+b);
 else if(Array.isArray(b)){assert(Array.isArray(a),where);assert.equal(a.length,b.length,where);for(let i=0;i<b.length;i++)equivalent(a[i],b[i],where+'['+i+']');}
 else if(b&&typeof b==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);for(const k of Object.keys(b))equivalent(a[k],b[k],where+'.'+k);}
 else assert.equal(a,b,where);
}
for(let i=0;i<actual.length;i++)try{equivalent(actual[i],expected[i],'case '+i);}catch(e){console.error('Request:',JSON.stringify(requests[i]));throw e;}
console.log(JSON.stringify({status:'passed',reference_blob:blob,cases:requests.length,map_cases:seeds.length*6,attack_cases:seeds.length*8,requested_v02314_verified:false},null,2));
