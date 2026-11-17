import fs from 'node:fs';
import vm from 'node:vm';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const raw=fs.readFileSync(path.join(root,'index.html'));
const blob=createHash('sha1').update(Buffer.from('blob '+raw.length+'\0')).update(raw).digest('hex');
const provenance=JSON.parse(fs.readFileSync(path.join(root,'native/data/provenance.json')));
assert.equal(blob,provenance.git_blob,'Review changed reference before extracting assets');
const html=raw.toString('utf8');
let source='const __modules=Object.create(null);\n';
for(const name of ['data.js','monster-kit.js','world-art.js','beginning-art.js']) {
 const a=html.indexOf('__modules["'+name+'"] = (');const b=html.indexOf('\n})();',a);
 assert(a>=0&&b>a,name);source+=html.slice(a,b+6)+'\n';
}
const gs=html.indexOf('class Geometry {'),ge=html.indexOf('\nfunction monsterGeometry',gs);
assert(gs>0&&ge>gs);
source+='const color=c=>Array.isArray(c)?c:[1,3,5].map(n=>parseInt(c.slice(n,n+2),16)/255);const scaleColor=(c,s)=>color(c).map(v=>Math.min(1,v*s));const norm=a=>{const l=Math.hypot(...a)||1;return a.map(v=>v/l);};const cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]];const dot=(a,b)=>a.reduce((s,x,i)=>s+x*b[i],0);\n';
source+=html.slice(gs,ge)+'\nglobalThis.api={Geometry,modules:__modules};';
const context=vm.createContext({});vm.runInContext(source,context,{timeout:10000});
const {Geometry,modules}=context.api,models={};
for(let id=0;id<5;id++){const g=new Geometry();modules['beginning-art.js'].drawBeginningMonster(g,id,0,0,0,1,0,0,0);models['monster-'+id]=g.a;}
const hero=new Geometry();modules['world-art.js'].explorer(hero,0,0,0,{weapon:null,shield:null});models.explorer=hero.a;
for(let id=0;id<9;id++){const g=new Geometry();modules['world-art.js'].lootModel(g,id,0);if(g.a.length)models['item-'+id]=g.a;}
const check=process.argv.includes('--check');
for(const [name,values] of Object.entries(models)) {
 const bytes=Buffer.alloc(8+values.length*4);bytes.write('CDM1');bytes.writeUInt32LE(values.length,4);
 values.forEach((v,i)=>bytes.writeFloatLE(v,8+i*4));
 const target=path.join(root,'native/assets/meshes/'+name+'.mesh');
 if(check)assert(bytes.equals(fs.readFileSync(target)),'mesh mismatch '+name);
 else {fs.mkdirSync(path.dirname(target),{recursive:true});fs.writeFileSync(target,bytes);}
}
const d=modules['data.js'],kit=modules['monster-kit.js'];
const data=JSON.stringify({families:d.FAMILIES,species:d.SPECIES,dungeons:d.DUNGEONS,themes:d.THEMES,items:d.ITEMS,skills:kit.SKILLS},null,2)+'\n';
const target=path.join(root,'native/data/game.json');
if(check)assert.equal(fs.readFileSync(target,'utf8'),data,'data extraction mismatch');
else fs.writeFileSync(target,data);
console.log((check?'Verified':'Exported')+' '+Object.keys(models).length+' reference meshes and game data; idle pose only.');
