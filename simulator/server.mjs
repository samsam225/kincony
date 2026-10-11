import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {randomBytes} from 'node:crypto';
import {Controller,validateConfig,clone,request,frame,checkResponse,decodeTs,decodeNh,safeEqual} from './core.mjs';
const root=path.dirname(path.dirname(fileURLToPath(import.meta.url)));
export function createSimulator({config=JSON.parse(fs.readFileSync(path.join(root,'config/example.json'))),password=randomBytes(18).toString('base64url'),persist=false}={}){
 const local=path.join(root,'simulator/state.local.json');if(persist&&fs.existsSync(local))config=JSON.parse(fs.readFileSync(local));
 const ctl=new Controller(config),csrf=randomBytes(24).toString('hex'),start=Date.now();let offset=0,nextPoll=0,nextBus=0,sensor=0,pending=null,lastSample=0;
 const faults={ts:'none',nh:'none'};let fixed=[27.3,56.7,423,6];
 const devices={ts:{address:1,baud:9600},nh:{address:1,baud:4800}};let staged=clone(devices),nhRestartRequired=false;
 const diagnostic={success:0,timeout:0,crc:0,exceptions:0,lastTx:'',lastRx:'',status:'Simulation',commissionDone:false,commissionVerified:false};
 const now=()=>Date.now()-start+offset,hex=b=>b.map(v=>v.toString(16).padStart(2,'0')).join(' ');
 const store=()=>{if(persist)fs.writeFileSync(local,JSON.stringify(ctl.config,null,2));};
 function acceptSensor(key,t){
  const nh=ctl.config.sensors.nh,isTs=key==='ts',fault=faults[key],req=request(ctl.config.sensors[key].address,3,isTs||nh.combined?0:2,isTs||nh.combined?3:1);diagnostic.lastTx=hex(req);
  if(fault==='stale')return;
  let words=isTs?[Math.round(fixed[0]*10)&65535,Math.round(fixed[1]*10),Math.round(fixed[2])]:nh.combined?[359,(-75)&65535,Math.round(fixed[3]*(nh.rangePpm===50?10:1))]:[Math.round(fixed[3]*(nh.rangePpm===50?10:1))];
  let reply=frame([req[0],3,words.length*2,...words.flatMap(n=>[n>>>8,n&255])]);
  if(fault==='crc')reply[reply.length-1]^=1;if(fault==='exception')reply=frame([req[0],131,2]);
  const err=fault==='timeout'?'Timeout':checkResponse(reply,req);diagnostic.lastRx=fault==='timeout'?'':hex(reply);diagnostic.status=err;
  if(err!=='OK'){if(err==='Timeout')diagnostic.timeout++;else if(err==='CRC invalide')diagnostic.crc++;else diagnostic.exceptions++;if(isTs)for(let i=0;i<3;i++)ctl.measures[i].error=err;else ctl.measures[3].error=err;return;}
  diagnostic.success++;
  if(isTs)decodeTs(reply.slice(3,-2)).forEach((v,i)=>ctl.accept(i,v,t));else{ctl.accept(3,decodeNh(reply.slice(3,-2),nh.rangePpm,nh.combined).nh3,t);if(!nh.modelConfirmed)ctl.measures[3].error='Non confirmé';else if(!ctl.warmReady(t))ctl.measures[3].error='Préchauffage';else if(ctl.measures[3].value>nh.rangePpm)ctl.measures[3].error='Hors plage';}
 }
 function tick(){const t=now();if(pending&&t>=pending.end){acceptSensor(pending.key,t);pending=null;nextBus=t+ctl.config.interTransactionMs;}if(!pending&&t>=nextBus&&(sensor===1||t>=nextPoll)){const key=sensor===0?'ts':'nh';pending={key,end:t+(faults[key]==='timeout'?ctl.config.responseTimeoutMs:20)};sensor=(sensor+1)%2;if(sensor===0)nextPoll=t+ctl.config.pollMs;}
  const before=ctl.relays.map(r=>r.command);ctl.tick(t);ctl.relays.forEach((r,i)=>{if(before[i]!==r.command)ctl.event(t,'relais',`R${i+1} commande ${r.command?'ON':'OFF'} / ${r.reason}`);});
  if(t-lastSample>=ctl.config.pollMs){lastSample=t;ctl.samples.push({t,v:ctl.measures.map((m,i)=>ctl.valid(i,t)?m.value:null)});if(ctl.samples.length>360)ctl.samples.shift();}}
 async function payload(req){let raw='';for await(const b of req){raw+=b;if(raw.length>16000)throw Error('Corps trop volumineux');}return JSON.parse(raw||'{}');}
 const server=http.createServer(async(req,res)=>{
  const send=(status,data,type='application/json')=>{res.writeHead(status,{'Content-Type':type+'; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(type==='application/json'?JSON.stringify(data):data);};
  const auth=req.headers.authorization||'';if(!safeEqual(auth,'Basic '+Buffer.from('admin:'+password).toString('base64'))){res.writeHead(401,{'WWW-Authenticate':'Basic realm="Serre A6v3 simulateur"'});res.end('Authentification requise');return;}
  try{
   const url=new URL(req.url,'http://localhost');if(req.method==='POST'&&!safeEqual(req.headers['x-csrf-token']||'',csrf))return send(403,'Jeton invalide','text/plain');
   if(req.method==='GET'){
    if(url.pathname==='/')return send(200,fs.readFileSync(path.join(root,'web/index.html'),'utf8'),'text/html');
    if(url.pathname==='/app.js')return send(200,fs.readFileSync(path.join(root,'web/app.js'),'utf8'),'text/javascript');
    if(url.pathname==='/api/config')return send(200,{config:ctl.config,csrf});
    if(url.pathname==='/api/state'){tick();const s=ctl.state(now());s.diagnostic={...s.diagnostic,...diagnostic,nhRestartRequired};return send(200,s);}
    if(url.pathname==='/api/history')return send(200,{samples:ctl.samples,events:ctl.events});
   }
   if(req.method==='POST'){
    const d=await payload(req),t=now();
    if(url.pathname==='/api/config'){ctl.configure(validateConfig(d),t);store();return send(200,'Enregistré ; observation et préchauffage à attester','text/plain');}
    if(url.pathname==='/api/mode'){ctl.setMode(d.mode,t);return send(200,'Mode changé','text/plain');}
    if(url.pathname==='/api/relay'){ctl.manualCommand(d.index,d.on,t);return send(200,'Demande enregistrée ; arbitrage applicable','text/plain');}
    if(url.pathname==='/api/pulse'){ctl.pulse(d.index,d.durationMs,t,d.benchConfirmed);return send(200,'Impulsion simulée lancée','text/plain');}
    if(url.pathname==='/api/warmup'){ctl.warmAction(d.action,t,d.attested);return send(200,'Préchauffage mis à jour','text/plain');}
    if(url.pathname==='/api/sim'){
     if(d.fault!==undefined){if(!['ts','nh'].includes(d.sensor)||!['none','timeout','crc','exception','stale'].includes(d.fault))throw Error('Défaut inconnu');faults[d.sensor]=d.fault;}
     if(d.advanceMs!==undefined){if(!Number.isInteger(d.advanceMs)||d.advanceMs<0||d.advanceMs>172800000)throw Error('Avance invalide');offset+=d.advanceMs;}
     if(d.network!==undefined){if(typeof d.network!=='boolean')throw Error('Réseau invalide');ctl.network=d.network;}
     if(d.values!==undefined){if(!Array.isArray(d.values)||d.values.length!==4||d.values.some(v=>!Number.isFinite(v))||d.values[0]<-40||d.values[0]>120||d.values[1]<0||d.values[1]>100||d.values[2]<0||d.values[2]>65535||d.values[3]<0||d.values[3]>ctl.config.sensors.nh.rangePpm)throw Error('Mesures de simulation hors plage');fixed=d.values;}
     if(d.restart===true){ctl.restart(t);pending=null;nextPoll=t;nextBus=t;sensor=0;}
     return send(200,'Simulation mise à jour','text/plain');
    }
    if(url.pathname==='/api/commission'){
     if(ctl.mode!=='observation'||ctl.relays.some(r=>r.command)||d.isolated!==true||!ctl.config.hardwareConfirmed)throw Error('Observation, sorties au repos, matériel confirmé, capteur isolé requis');
     const ts=d.sensor==='ts';if(!['ts','nh'].includes(d.sensor)||!['read-current','set-address','set-baud','power-cycle','verify'].includes(d.action))throw Error('Commande inconnue');
     if(!Number.isInteger(d.currentAddress)||d.currentAddress<1||d.currentAddress>247||![1200,2400,4800,9600,19200,38400,57600,115200].includes(d.currentBaud))throw Error('Paramètres actuels invalides');
     if(d.action==='power-cycle'){devices[d.sensor]=clone(staged[d.sensor]);diagnostic.status='Redémarrage électrique simulé';}
     else{
      if(d.action==='verify'&&!ts&&nhRestartRequired&&d.powerRestartConfirmed!==true)throw Error('Redémarrage électrique requis');
      let address=d.currentAddress,baud=d.currentBaud,fn=3,reg=ts?0x4000:0x07d0,value=2;
      if(d.action==='set-address'){fn=6;value=ts?1:2;}if(d.action==='set-baud'){fn=6;reg++;value=ts?3:2;}if(d.action==='verify'){address=ts?1:2;baud=9600;}
      const req=request(address,fn,reg,value);diagnostic.lastTx=hex(req);
      if(devices[d.sensor].address!==address||devices[d.sensor].baud!==baud)throw Error('Timeout simulé : paramètres matériels différents');
      if(fn===6){if(d.action==='set-address')staged[d.sensor].address=value;else staged[d.sensor].baud=9600;if(!ts)nhRestartRequired=true;diagnostic.lastRx=hex(req);}
      else{const p=frame([address,3,4,0,devices[d.sensor].address,0,devices[d.sensor].baud===9600?(ts?3:2):1]);diagnostic.lastRx=hex(p);diagnostic.commissionVerified=d.action==='verify'&&devices[d.sensor].address===(ts?1:2)&&devices[d.sensor].baud===9600;if(diagnostic.commissionVerified&&!ts)nhRestartRequired=false;}
      diagnostic.status='OK simulé';
     }
     diagnostic.commissionDone=true;ctl.config.busCommissioned=false;store();ctl.event(t,'mise en service',d.sensor+' / '+d.action+' (simulée)');return send(200,'Commande simulée ; consulter diagnostic','text/plain');
    }
   }
   send(404,'Route inconnue','text/plain');
  }catch(e){send(400,e.message,'text/plain');}
 });
 const timer=setInterval(tick,20);server.on('close',()=>clearInterval(timer));return {server,password,csrf,ctl,tick,now};
}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const app=createSimulator({password:process.env.SIM_PASSWORD||undefined,persist:true});const port=Number(process.env.PORT||8080);app.server.listen(port,'127.0.0.1',()=>{console.log(`Simulation locale : http://127.0.0.1:${port}\nUtilisateur : admin\nMot de passe de cette session : ${app.password}\nAucune connexion à un équipement réel.`);});
}