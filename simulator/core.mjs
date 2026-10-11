import { timingSafeEqual } from 'node:crypto';
export const clone=x=>JSON.parse(JSON.stringify(x));
export function crc16(bytes){let c=65535;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=c&1?(c>>>1)^0xa001:c>>>1;}return c;}
export function frame(bytes){const c=crc16(bytes);return [...bytes,c&255,c>>>8];}
export const word=(a,i=0)=>(a[i]<<8)|a[i+1];
export const signed=n=>n&0x8000?n-65536:n;
export function request(address,fn,reg,value){return frame([address,fn,reg>>>8,reg&255,value>>>8,value&255]);}
export function checkResponse(p,req){if(p.length<5)return 'Longueur incorrecte';if(crc16(p.slice(0,-2))!==word([p.at(-1),p.at(-2)]))return 'CRC invalide';if(p[0]!==req[0])return 'Adresse inattendue';if(p[1]===(req[1]|128))return p.length===5?'Exception Modbus '+p[2]:'Longueur incorrecte';if(p[1]!==req[1])return 'Fonction inattendue';if(req[1]===3&&(p[2]!==word(req,4)*2||p.length!==p[2]+5))return 'Longueur incorrecte';if(req[1]===6&&(p.length!==8||p.slice(2,6).some((v,i)=>v!==req[i+2])))return 'Longueur incorrecte';return 'OK';}
export const decodeTs=a=>[signed(word(a))/10,word(a,2)/10,word(a,4)];
export const decodeNh=(a,range,combined=false)=>combined?{humidity:word(a)/10,temperature:signed(word(a,2))/10,nh3:word(a,4)/(range===50?10:1)}:{nh3:word(a)/(range===50?10:1)};
export const warmupMs=range=>range===500?172800000:300000;
export const t15=baud=>baud>19200?750:Math.ceil(16500000/baud);
export const t35=baud=>baud>19200?1750:Math.ceil(38500000/baud);
export function safeEqual(a,b){const x=Buffer.from(a),y=Buffer.from(b);return x.length===y.length&&timingSafeEqual(x,y);}
const roles=['vent_main','vent_extra','heating','humidifier','cooling','lighting','alarm','unused'];
const policies=['off','on','hold'];
export function activeProfile(c){return c.profiles.find(p=>p.species===c.species&&c.ageDays>=p.minAgeDays&&c.ageDays<=p.maxAgeDays);}
export function validateConfig(c){
 const fail=s=>{throw Error(s);};const integer=(v,min,max)=>Number.isInteger(v)&&v>=min&&v<=max;
 if(!c||c.version!==1||typeof c.hardwareConfirmed!=='boolean'||typeof c.busCommissioned!=='boolean')fail('Configuration matérielle invalide');
 if(typeof c.species!=='string'||!c.species.length||c.species.length>80||!integer(c.ageDays,0,9999))fail('Espèce ou âge invalide');
 if(!integer(c.staleMs,1000,600000)||!integer(c.pollMs,1000,60000)||c.staleMs<=c.pollMs||!integer(c.responseTimeoutMs,200,5000)||!integer(c.interTransactionMs,200,5000))fail('Délais invalides : interrogation et réponse ≥200 ms, péremption > cycle');
 for(const s of [c.sensors?.ts,c.sensors?.nh])if(!s||!integer(s.address,1,247)||![1200,2400,4800,9600,19200,38400,57600,115200].includes(s.baud))fail('Paramètres Modbus invalides');
 if(c.sensors.ts.address===c.sensors.nh.address||c.sensors.ts.baud!==c.sensors.nh.baud)fail('Bus partagé : adresses distinctes et vitesse commune');
 const nh=c.sensors.nh;if(![50,100,500,1000].includes(nh.rangePpm)||typeof nh.combined!=='boolean'||typeof nh.modelConfirmed!=='boolean'||typeof nh.model!=='string'||!nh.model.length||nh.model.length>100)fail('Modèle NH3 invalide');
 if(!Array.isArray(c.profiles)||!c.profiles.length||c.profiles.length>12)fail('1 à 12 profils requis');
 for(let i=0;i<c.profiles.length;i++){
  const p=c.profiles[i];if(typeof p.species!=='string'||!p.species.length||p.species.length>80||!integer(p.minAgeDays,0,9999)||!integer(p.maxAgeDays,p.minAgeDays,9999)||typeof p.validated!=='boolean')fail('Profil avicole invalide');
  for(const k of ['hot','humid','co2','nh3','cold','dry']){const t=p.rules?.[k];if(!t||!Number.isFinite(t.value)||!Number.isFinite(t.hysteresis)||t.hysteresis<=0)fail('Seuil ou hystérésis invalide');}
  const r=p.rules;if(r.cold.value+r.cold.hysteresis>=r.hot.value-r.hot.hysteresis||r.dry.value+r.dry.hysteresis>=r.humid.value-r.humid.hysteresis)fail('Bandes de chauffage/refroidissement ou humidité en conflit');
  if(r.cold.value<-40||r.hot.value>120||r.dry.value<0||r.humid.value>100||r.co2.value<0||r.co2.value>65535||r.nh3.value<0||r.nh3.value>nh.rangePpm)fail('Consigne hors plage de configuration');
  if(!integer(r.minVentPeriodMs,1000,86400000)||!integer(r.minVentOnMs,0,r.minVentPeriodMs))fail('Cycle de ventilation invalide');
  for(let j=0;j<i;j++){const q=c.profiles[j];if(p.species===q.species&&p.minAgeDays<=q.maxAgeDays&&q.minAgeDays<=p.maxAgeDays)fail('Profils de même espèce se chevauchant');}
 }
 if(!activeProfile(c))fail('Aucun profil pour cette espèce et cet âge');
 if(!Array.isArray(c.relays)||c.relays.length!==6)fail('Six relais requis');
 const assigned=new Set();for(const r of c.relays){if(typeof r.name!=='string'||!r.name.length||r.name.length>80||!roles.includes(r.role)||typeof r.confirmed!=='boolean'||typeof r.policyConfirmed!=='boolean'||!integer(r.minOnMs,0,3600000)||!integer(r.minOffMs,0,3600000)||!policies.includes(r.startup)||!policies.includes(r.sensorFault)||!policies.includes(r.observation)||r.network!=='continue_local')fail('Affectation ou politique de relais invalide');if(r.role!=='unused'){if(assigned.has(r.role))fail('Une seule sortie par rôle');assigned.add(r.role);}}
 return clone(c);
}
export class Controller {
 constructor(config){this.config=validateConfig(config);this.events=[];this.samples=[];this.restart(0);}
 event(t,type,detail){this.events.push({t,type,detail});if(this.events.length>512)this.events.shift();}
 restart(now){this.mode='observation';this.startup=true;this.network=true;this.manual=Array(6).fill(false);this.pulseIndex=-1;this.pulseEnd=0;this.latches={hot:false,humid:false,co2:false,nh3:false,cold:false,dry:false};this.measures=Array.from({length:4},()=>({value:null,last:0,seen:false,error:'Non confirmé'}));this.relays=Array.from({length:6},()=>({command:false,changed:now,reason:'Non confirmé',writeAcknowledged:false,physicalState:null}));this.warm={started:false,start:now,ready:false};this.samples=[];this.lastAlarm='';this.event(now,'redémarrage','Observation ; commandes manuelles oubliées ; préchauffage à attester');}
 configure(c,now){this.config=validateConfig(c);this.restart(now);this.event(now,'configuration','Configuration enregistrée ; réarmement explicite requis');}
 setMode(m,now){if(!['observation','manual','automatic'].includes(m))throw Error('Mode inconnu');if(m==='automatic'&&(!activeProfile(this.config).validated||!this.config.hardwareConfirmed||!this.config.busCommissioned||!this.config.sensors.nh.modelConfirmed))throw Error('Confirmer matériel, bus, modèle NH3 et consignes avant automatisation');this.mode=m;this.startup=false;this.manual.fill(false);this.pulseIndex=-1;this.event(now,'mode',m);}
 accept(i,v,now){this.measures[i]={value:v,last:now,seen:true,error:'OK'};}
 valid(i,now){const m=this.measures[i];return m.seen&&Number.isFinite(m.value)&&m.error==='OK'&&now-m.last<this.config.staleMs;}
 warmReady(now){return this.warm.ready||(this.warm.started&&now-this.warm.start>=warmupMs(this.config.sensors.nh.rangePpm));}
 warmAction(action,now,attested){if(action==='start')this.warm={started:true,start:now,ready:false};else if(action==='ready'&&attested===true)this.warm.ready=true;else throw Error('Attestation de préchauffage requise');this.measures[3].error='Préchauffage';this.event(now,'préchauffage',action);}
 manualCommand(i,on,now){const r=this.config.relays[i];if(this.mode!=='manual'||!r?.confirmed||!r.policyConfirmed||typeof on!=='boolean')throw Error('Relais ou mode manuel non confirmé');this.manual[i]=on;this.event(now,'commande manuelle',`R${i+1} ${on}`);}
 pulse(i,ms,now,bench){const c=this.config.relays[i];if(bench!==true||this.mode!=='observation'||!c?.confirmed||!c.policyConfirmed||c.observation!=='off'||!Number.isInteger(ms)||ms<100||ms>2000||this.pulseIndex>=0||this.relays.some(r=>r.command))throw Error('Impulsion : observation, banc attesté, relais confirmé, 100–2000 ms, toutes sorties au repos');this.pulseIndex=i;this.pulseEnd=now+ms;this.startup=false;this.event(now,'test relais',`R${i+1}, ${ms} ms`);}
 tick(now){
  const c=this.config,rules=activeProfile(c).rules,bad=this.measures.some((_,i)=>!this.valid(i,now));
  const index={hot:0,humid:1,co2:2,nh3:3,cold:0,dry:1};for(const k in index){const t=rules[k],v=this.measures[index[k]].value,l=this.latches[k];this.latches[k]=bad?false:['cold','dry'].includes(k)?(l?v<t.value+t.hysteresis:v<=t.value):(l?v>t.value-t.hysteresis:v>=t.value);}
  const l=this.latches,boost=l.hot||l.humid||l.co2||l.nh3,minimum=now%rules.minVentPeriodMs<rules.minVentOnMs;
  const stopped=this.pulseIndex>=0&&now>=this.pulseEnd?this.pulseIndex:-1;if(stopped>=0)this.pulseIndex=-1;
  const policy=(p,v)=>p==='on'||(p==='hold'&&v);
  for(let i=0;i<6;i++){
   const rc=c.relays[i],s=this.relays[i];let want=false,force=false,why='Automatique';
   if(!rc.confirmed||!rc.policyConfirmed||rc.role==='unused'){why='Non confirmé';force=true;}
   else if(this.startup){want=policy(rc.startup,s.command);why='Démarrage';force=true;}
   else if(this.pulseIndex===i){want=true;why='Impulsion de test';force=true;}
   else if(stopped===i){why='Impulsion de test';force=true;}
   else if(this.mode==='observation'){want=policy(rc.observation,s.command);why='Observation';force=true;}
   else if(bad){want=policy(rc.sensorFault,s.command);why='Défaut capteur';force=true;}
   else if(this.mode==='manual'){want=this.manual[i];why='Manuel';}
   else if(!activeProfile(c).validated){why='Observation';force=true;}
   else want=({vent_main:minimum||boost,vent_extra:boost,heating:l.cold&&!boost,humidifier:l.dry&&!boost&&!l.cold,cooling:l.hot&&!l.cold,lighting:false,alarm:boost,unused:false})[rc.role];
   const on=role=>c.relays.some((r,j)=>r.confirmed&&r.role===role&&this.relays[j].command),vent=on('vent_main')||on('vent_extra'),heat=on('heating'),cool=on('cooling');
   if((want||s.command)&&((rc.role==='heating'&&(boost||vent||cool))||(rc.role==='humidifier'&&(boost||vent||heat||cool))||(rc.role==='cooling'&&heat))){want=false;force=true;why='Conflit prioritaire';}
   if(want!==s.command){const duration=s.command?rc.minOnMs:rc.minOffMs;if(force||now-s.changed>=duration){s.command=want;s.changed=now;s.writeAcknowledged=false;}else why='Temporisation';}s.reason=why;
  }
  const on=role=>c.relays.some((r,j)=>r.role===role&&this.relays[j].command),vent=on('vent_main')||on('vent_extra'),heat=on('heating'),cool=on('cooling');
  for(let i=0;i<6;i++){const s=this.relays[i],role=c.relays[i].role;if(s.command&&((role==='heating'&&(vent||cool))||(role==='humidifier'&&(vent||heat||cool)))){s.command=false;s.changed=now;s.reason='Conflit prioritaire';s.writeAcknowledged=false;}}
  const alarm=bad?'Mesure invalide : politique de défaut appliquée':boost?'Consigne dépassée':'';if(alarm!==this.lastAlarm){this.event(now,'alarme',alarm||'Retour à la normale');this.lastAlarm=alarm;}
 }
 state(now){return {simulation:true,mode:this.mode,network:this.network,uptimeMs:now,alarm:this.lastAlarm,warmup:{started:this.warm.started,ready:this.warmReady(now),remainingMs:this.warmReady(now)?0:this.warm.started?Math.max(0,warmupMs(this.config.sensors.nh.rangePpm)-(now-this.warm.start)):null},values:this.measures.map((m,i)=>({name:['Température','Humidité','CO₂','NH₃'][i],unit:['°C','%RH','ppm','ppm'][i],value:m.value,valid:this.valid(i,now),ageMs:m.seen?now-m.last:null,status:m.error==='OK'&&!this.valid(i,now)?'Périmé':m.error})),relays:this.relays.map((r,i)=>({...r,name:this.config.relays[i].name,role:this.config.relays[i].role,confirmed:this.config.relays[i].confirmed,mode:this.mode})),diagnostic:{hardwareConfirmed:this.config.hardwareConfirmed,busCommissioned:this.config.busCommissioned}};}
}