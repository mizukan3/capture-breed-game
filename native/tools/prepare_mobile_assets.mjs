import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const destination=path.resolve(process.argv[2]||path.join(root,'build-mobile/assets'));
fs.mkdirSync(destination,{recursive:true});
fs.cpSync(path.join(root,'native/licenses'),path.join(destination,'licenses'),{recursive:true});fs.cpSync(path.join(root,'native/assets'),destination,{recursive:true});
const font=path.join(destination,'fonts/NotoSansJP.ttf');
if(!fs.existsSync(font)) {
 const url='https://raw.githubusercontent.com/google/fonts/295d98a7a0c17c68f1341eaeea354e7960ea70d3/ofl/notosansjp/NotoSansJP%5Bwght%5D.ttf';
 const response=await fetch(url);if(!response.ok)throw new Error('font download: '+response.status);
 const buffer=Buffer.from(await response.arrayBuffer());
 const blob=createHash('sha1').update(Buffer.from('blob '+buffer.length+'\0')).update(buffer).digest('hex');
 if(blob!=='cdd8f083c1f5928ff3361f8cda4d3fc9462cbe89')throw new Error('Pinned font content mismatch');
 fs.writeFileSync(font,buffer);
}
if(!process.argv.includes('--metal'))for(const name of ['scene.vert','scene.frag','ui.vert','ui.frag']) {
 const r=spawnSync('glslangValidator',['-V',path.join(root,'native/assets/shaders',name),'-o',path.join(destination,'shaders',name+'.spv')],{stdio:'inherit'});
 if(r.status!==0)throw new Error('Install host glslangValidator before preparing Vulkan shaders');
}
console.log(destination);
