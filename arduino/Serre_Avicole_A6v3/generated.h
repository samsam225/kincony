#pragma once
static const char PAGE[] PROGMEM = R"AVUI(<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Serre avicole · A6v3</title><style>
:root{font-family:system-ui,sans-serif;color:#152d29;background:#eff4f1;color-scheme:light}*{box-sizing:border-box}body{margin:0}header{background:#123d32;color:white;padding:24px max(18px,calc((100vw - 1140px)/2));display:flex;justify-content:space-between;gap:16px;align-items:center}h1{font-size:24px;margin:0 0 6px}h2{font-size:19px;margin:0 0 16px}p{line-height:1.5}main{max-width:1180px;margin:auto;padding:20px}section{background:#fff;border:1px solid #dce6df;border-radius:14px;padding:20px;margin-bottom:18px}.grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px}.relays{grid-template-columns:repeat(3,minmax(0,1fr))}.card{padding:16px;border:1px solid #dce6df;border-radius:10px}.value{font-size:30px;font-weight:700;color:#145640}.muted{color:#5e7068;font-size:14px}.bad{color:#9a332c}.good{color:#14604b}.pill{border-radius:20px;padding:6px 12px;background:#e4f0e8;display:inline-block}.warning{background:#fff4db;border:1px solid #e2ce8c;padding:14px;border-radius:10px;margin-bottom:18px}.toolbar{display:flex;flex-wrap:wrap;gap:8px;align-items:center}button,.link{border:1px solid #b5c9be;border-radius:7px;background:#f3f8f4;color:#123d32;padding:10px 12px;cursor:pointer;font:inherit}button:hover{background:#dfede4}button:disabled{opacity:.45;cursor:default}input,select,textarea{font:inherit;border:1px solid #b5c9be;padding:9px;border-radius:6px;max-width:100%}input[type=number]{width:130px}label{display:flex;gap:8px;align-items:center;margin:8px 0}textarea{width:100%;min-height:280px;font-family:monospace;font-size:13px}table{width:100%;border-collapse:collapse}th,td{text-align:left;padding:10px;border-bottom:1px solid #e3e9e5}details>summary{cursor:pointer;font-weight:600;padding:8px 0}pre{overflow:auto;background:#f4f7f4;padding:12px;white-space:pre-wrap}canvas{width:100%;height:200px}#message{position:sticky;bottom:12px;padding:12px;background:#123d32;color:#fff;border-radius:8px;display:none}#message.visible{display:block}.scroll{overflow:auto}small{display:block;margin:8px 0}.tag{font-size:13px;font-weight:600}@media(max-width:720px){.grid{grid-template-columns:repeat(2,minmax(0,1fr))}.relays{grid-template-columns:1fr}header{align-items:flex-start;flex-direction:column}main{padding:12px}section{padding:15px}.value{font-size:26px}th,td{padding:8px}h1{font-size:22px}}
</style></head><body><header><div><h1>Serre avicole</h1><div>KinCony A6v3 · Contrôle local</div></div><div id="modeBadge" class="tag">Connexion…</div></header><main>
<div id="simulation" class="warning" hidden>SIMULATION — aucune connexion au matériel. Les consignes affichées sont des exemples logiciels à valider selon l’espèce, l’âge et les équipements.</div>
<p id="network" class="muted">Chargement de l’état…</p><section><h2>Mesures</h2><div id="measurements" class="grid"></div><p id="alarm" role="alert" class="bad"></p><p id="warmStatus" class="muted"></p><div class="toolbar"><button id="warmStart">Démarrer le préchauffage à la mise sous tension NH3</button><label><input type="checkbox" id="warmAttested">Je confirme que le NH3 est resté alimenté et a terminé son préchauffage</label><button id="warmReady">Attester : capteur prêt</button></div></section>
<section><h2>Six relais</h2><div class="toolbar"><button data-mode="observation">Observation</button><button data-mode="manual">Manuel</button><button data-mode="automatic">Automatique</button></div><p class="muted">Seul l’état commandé est connu. Un acquittement I²C ne confirme pas le fonctionnement d’un ventilateur ou d’un chauffage. Les sorties non confirmées restent bloquées. Les politiques de défaut peuvent prendre priorité sur le mode manuel.</p><div id="relays" class="grid relays"></div><label><input id="bench" type="checkbox">Test sur banc : charges déconnectées, toutes les sorties au repos</label><label>Durée d’impulsion (ms)<input id="pulseMs" type="number" min="100" max="2000" value="500"></label></section>
<section><h2>Consignes selon l’espèce et l’âge</h2><p class="warning">Ces valeurs ne constituent pas des recommandations avicoles validées. L’automatisation exige une validation explicite des consignes, du matériel, du bus et du modèle NH3.</p><label>Espèce<input id="species" maxlength="80"></label><label>Âge (jours)<input id="ageDays" type="number" min="0" max="9999"></label><button id="selectProfile">Appliquer l’espèce et l’âge</button><div class="scroll"><table><thead><tr><th>Déclenchement</th><th>Seuil</th><th>Hystérésis</th></tr></thead><tbody id="thresholds"></tbody></table></div><label>Période ventilation minimale (s)<input id="ventPeriod" type="number" min="1"></label><label>Temps de ventilation minimale (s)<input id="ventOn" type="number" min="0"></label><label><input id="profileValidated" type="checkbox">Consignes validées pour ce profil d’espèce et d’âge</label><button id="saveRules">Enregistrer les consignes</button><details><summary>Affectations, politiques, profils d’âge et configuration complète</summary><p>Le JSON permet de régler les noms, rôles, confirmations, temporisations et politiques par sortie. R4 doit être choisi comme humidification ou refroidissement selon l’équipement réel. « hold » conserve une commande depuis le dernier changement ; au redémarrage elle vaut OFF. Le réseau utilise « continue_local ».</p><textarea id="configJson" spellcheck="false"></textarea><button id="saveConfig">Valider et enregistrer le JSON</button></details></section>
<section><h2>Historique et journal</h2><label>Mesure<select id="plot"><option>Température</option><option>Humidité</option><option>CO₂</option><option>NH₃</option></select></label><canvas id="chart"></canvas><button id="export">Exporter les mesures en CSV</button><div class="scroll"><table><thead><tr><th>Depuis le démarrage</th><th>Événement</th><th>Détail</th></tr></thead><tbody id="events"></tbody></table></div></section>
<section><details><summary>Diagnostic Modbus et mise en service</summary><pre id="diagnostic"></pre><p>Un seul capteur connecté, un seul maître. Les commandes ci-dessous écrivent uniquement l’adresse et la vitesse, jamais l’étalonnage. Reconnecter les deux seulement après relecture de chaque cible : TS85-B 1 / 9600 et NH3 2 / 9600, 8N1.</p><label>Capteur<select id="commissionSensor"><option value="ts">TS85-B</option><option value="nh">NH3</option></select></label><label>Adresse actuelle<input id="currentAddress" type="number" min="1" max="247" value="1"></label><label>Vitesse actuelle<select id="currentBaud"><option>1200</option><option>2400</option><option>4800</option><option selected>9600</option><option>19200</option><option>38400</option><option>57600</option><option>115200</option></select></label><label><input id="isolated" type="checkbox">Seul le capteur choisi est connecté ; sorties au repos</label><label><input id="powerRestart" type="checkbox">Redémarrage électrique NH3 réalisé après configuration</label><div class="toolbar"><button data-commission="read-current">Lire les paramètres actuels</button><button data-commission="set-address">Écrire l’adresse cible</button><button data-commission="set-baud">Écrire 9600 bauds</button><button data-commission="verify">Relire et vérifier la cible</button><button id="simPower" data-commission="power-cycle" hidden>Simuler le redémarrage du capteur</button></div><p class="muted">Après validation des relectures et du câblage, confirmer busCommissioned dans le JSON. Le profil par défaut n’actionne aucun équipement.</p></details></section>
<section id="simTools" hidden><h2>Banc de simulation</h2><label>Capteur<select id="faultSensor"><option value="ts">TS85-B</option><option value="nh">NH3</option></select></label><label>Défaut<select id="fault"><option value="none">Aucun</option><option value="timeout">Timeout</option><option value="crc">CRC invalide</option><option value="exception">Exception Modbus</option><option value="stale">Données périmées</option></select></label><div class="toolbar"><button id="inject">Injecter le défaut</button><button id="advance">Avancer de 48 h</button><button id="offline">Simuler la perte réseau</button><button id="restart">Simuler un redémarrage</button></div><label>Température °C<input id="simTemp" type="number" value="27.3" step="0.1"></label><label>Humidité %RH<input id="simHum" type="number" value="56.7" step="0.1"></label><label>CO₂ ppm<input id="simCo2" type="number" value="423"></label><label>NH₃ ppm<input id="simNh" type="number" value="6" step="0.1"></label><button id="simValues">Appliquer ces mesures simulées</button></section>
<section><details><summary>Connexion Wi-Fi locale</summary><p>Le point d’accès reste disponible et le contrôle local continue sans Internet.</p><label>SSID<input id="ssid" maxlength="32"></label><label>Mot de passe<input id="wifiPassword" type="password" maxlength="63" autocomplete="new-password"></label><button id="wifiSave">Connecter au réseau local</button></details></section><div id="message" role="status" aria-live="polite"></div></main><script>'use strict';
const $=id=>document.getElementById(id);let config,token='',stateData,historyData={samples:[],events:[]};
const ruleNames={hot:'Température haute (°C)',humid:'Humidité haute (%RH)',co2:'CO₂ haut (ppm)',nh3:'NH₃ haut (ppm)',cold:'Chauffage sous (°C)',dry:'Humidification sous (%RH)'};
const el=(tag,text)=>{const n=document.createElement(tag);if(text!==undefined)n.textContent=text;return n;};
function message(text,bad=false){$('message').textContent=text;$('message').className='visible';$('message').style.background=bad?'#9a332c':'#123d32';}
async function get(url){const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw Error(await r.text());return r.json();}
async function post(url,data){const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json','X-CSRF-Token':token},body:JSON.stringify(data)});const text=await r.text();if(!r.ok)throw Error(text);message(text);await refresh();}
function profile(c=config){return c.profiles.find(p=>p.species===c.species&&c.ageDays>=p.minAgeDays&&c.ageDays<=p.maxAgeDays);}
function renderConfig(){const p=profile();$('species').value=config.species;$('ageDays').value=config.ageDays;$('profileValidated').checked=p.validated;$('ventPeriod').value=p.rules.minVentPeriodMs/1000;$('ventOn').value=p.rules.minVentOnMs/1000;$('thresholds').replaceChildren();for(const [k,name] of Object.entries(ruleNames)){const row=el('tr');row.append(el('td',name));for(const field of ['value','hysteresis']){const cell=el('td'),input=el('input');input.type='number';input.step='0.1';input.id=k+'_'+field;input.value=p.rules[k][field];cell.append(input);row.append(cell);}$('thresholds').append(row);}$('configJson').value=JSON.stringify(config,null,2);}
async function refresh(){try{stateData=await get('/api/state');const s=stateData;$('simulation').hidden=!s.simulation;$('simTools').hidden=!s.simulation;$('simPower').hidden=!s.simulation;$('modeBadge').textContent=s.mode==='automatic'?'AUTOMATIQUE':s.mode==='manual'?'MANUEL':'OBSERVATION';$('network').textContent=s.simulation?'Simulation locale ; aucun équipement connecté':s.network?'Wi-Fi local connecté · le contrôle reste autonome':'Point d’accès local · Internet non requis';$('alarm').textContent=s.alarm||'';$('warmStatus').textContent=s.warmup.ready?'NH3 : préchauffage terminé ou attesté':s.warmup.started?'NH3 : préchauffage restant '+(s.warmup.remainingMs/3600000).toFixed(2)+' h':'NH3 : début du préchauffage non attesté';$('measurements').replaceChildren();s.values.forEach(v=>{const card=el('article');card.className='card';card.append(el('div',v.name));const value=el('div',v.value===null?'—':Number(v.value).toFixed(v.unit==='ppm'?1:1)+' '+v.unit);value.className='value';card.append(value);const status=el('p',v.status+(v.valid?' · valide':' · non valide'));status.className=v.valid?'good':'bad';card.append(status,el('small',v.ageMs===null?'Aucune réception':'Dernière réception : '+(v.ageMs/1000).toFixed(1)+' s'));$('measurements').append(card);});$('relays').replaceChildren();s.relays.forEach((r,i)=>{const card=el('article');card.className='card';card.append(el('h2','R'+(i+1)+' · '+r.name),el('p',r.command?'Commande : ON':'Commande : OFF'),el('small',r.reason),el('small','Marche minimale : '+config.relays[i].minOnMs/1000+' s · Arrêt minimal : '+config.relays[i].minOffMs/1000+' s'),el('small',r.confirmed?'Affectation confirmée':'Affectation non confirmée'),el('small',r.writeAcknowledged?'Écriture I²C acquittée ; équipement non vérifié':'État physique inconnu'));const actions=el('div');actions.className='toolbar';for(const [label,action] of [['ON',true],['OFF',false]]){const b=el('button',label);b.disabled=!r.confirmed||s.mode!=='manual';b.onclick=()=>post('/api/relay',{index:i,on:action}).catch(report);actions.append(b);}const test=el('button','Impulsion de test');test.disabled=!r.confirmed||s.mode!=='observation';test.onclick=()=>post('/api/pulse',{index:i,durationMs:Number($('pulseMs').value),benchConfirmed:$('bench').checked}).catch(report);actions.append(test);card.append(actions);$('relays').append(card);});$('diagnostic').textContent=JSON.stringify(s.diagnostic,null,2);}catch(e){$('network').textContent='Interface inaccessible : état matériel inconnu';$('measurements').querySelectorAll('article').forEach(n=>{n.classList.add('bad');const status=n.querySelector('p');if(status){status.textContent='Connexion perdue · non valide';status.className='bad';}});report(e);}}
const report=e=>message(e.message,true);
async function history(){try{historyData=await get('/api/history');draw();$('events').replaceChildren();for(const e of historyData.events.slice(-40).reverse()){const r=el('tr');r.append(el('td',(e.t/1000).toFixed(1)+' s'),el('td',e.type),el('td',e.detail));$('events').append(r);}}catch(e){report(e);}}
function draw(){const c=$('chart'),dpr=devicePixelRatio||1,w=c.getBoundingClientRect().width;c.width=w*dpr;c.height=200*dpr;const ctx=c.getContext('2d');ctx.scale(dpr,dpr);const idx=$('plot').selectedIndex,rows=historyData.samples,vals=rows.map(s=>s.v[idx]).filter(v=>v!==null&&Number.isFinite(v));ctx.font='12px system-ui';ctx.fillStyle='#5e7068';if(vals.length<2){ctx.fillText('En attente de deux mesures valides. Les défauts créent des interruptions.',12,30);return;}let low=Math.min(...vals),high=Math.max(...vals);if(low===high){low--;high++;}const first=rows[0].t,last=rows.at(-1).t;ctx.fillText(high.toFixed(1),5,20);ctx.fillText(low.toFixed(1),5,170);ctx.strokeStyle='#14604b';ctx.lineWidth=2;ctx.beginPath();let connected=false;for(const r of rows){if(r.v[idx]===null){connected=false;continue;}const x=50+(r.t-first)/Math.max(1,last-first)*(w-60),y=25+(high-r.v[idx])/(high-low)*140;if(connected)ctx.lineTo(x,y);else ctx.moveTo(x,y);connected=true;}ctx.stroke();ctx.fillText(((last-first)/60000).toFixed(1)+' min affichées',50,194);}
$('selectProfile').onclick=async()=>{try{const c=structuredClone(config);c.species=$('species').value;c.ageDays=Number($('ageDays').value);await post('/api/config',c);config=c;renderConfig();}catch(e){report(e);}};
$('saveRules').onclick=async()=>{try{if($('species').value!==config.species||Number($('ageDays').value)!==config.ageDays)throw Error('Appliquer d’abord l’espèce et l’âge pour afficher les bonnes consignes');const c=structuredClone(config),p=profile(c);for(const k of Object.keys(ruleNames))for(const field of ['value','hysteresis'])p.rules[k][field]=Number($(k+'_'+field).value);p.rules.minVentPeriodMs=Number($('ventPeriod').value)*1000;p.rules.minVentOnMs=Number($('ventOn').value)*1000;p.validated=$('profileValidated').checked;await post('/api/config',c);config=c;renderConfig();}catch(e){report(e);}};
$('saveConfig').onclick=async()=>{try{const c=JSON.parse($('configJson').value);await post('/api/config',c);config=c;renderConfig();}catch(e){report(e);}};
document.querySelectorAll('[data-mode]').forEach(b=>b.onclick=()=>post('/api/mode',{mode:b.dataset.mode}).catch(report));
$('warmStart').onclick=()=>post('/api/warmup',{action:'start'}).catch(report);$('warmReady').onclick=()=>post('/api/warmup',{action:'ready',attested:$('warmAttested').checked}).catch(report);
document.querySelectorAll('[data-commission]').forEach(b=>b.onclick=()=>post('/api/commission',{sensor:$('commissionSensor').value,action:b.dataset.commission,currentAddress:Number($('currentAddress').value),currentBaud:Number($('currentBaud').value),isolated:$('isolated').checked,powerRestartConfirmed:$('powerRestart').checked}).catch(report));
$('commissionSensor').onchange=()=>{$('currentAddress').value=1;$('currentBaud').value=$('commissionSensor').value==='nh'?4800:9600;};
$('inject').onclick=()=>post('/api/sim',{sensor:$('faultSensor').value,fault:$('fault').value}).catch(report);$('advance').onclick=()=>post('/api/sim',{advanceMs:172800000}).catch(report);$('offline').onclick=()=>post('/api/sim',{network:!stateData.network}).catch(report);$('restart').onclick=()=>post('/api/sim',{restart:true}).catch(report);$('simValues').onclick=()=>post('/api/sim',{values:['simTemp','simHum','simCo2','simNh'].map(id=>Number($(id).value))}).catch(report);
$('wifiSave').onclick=()=>post('/api/wifi',{ssid:$('ssid').value,password:$('wifiPassword').value}).then(()=>{$('wifiPassword').value='';}).catch(report);
$('export').onclick=()=>{const csv='secondes;temperature_C;humidite_pct;co2_ppm;nh3_ppm\n'+historyData.samples.map(s=>[s.t/1000,...s.v.map(v=>v===null?'':v)].join(';')).join('\n');const url=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8'})),a=el('a');a.href=url;a.download='mesures.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),5000);};
$('plot').onchange=draw;window.addEventListener('resize',draw);
(async()=>{try{const d=await get('/api/config');config=d.config;token=d.csrf;renderConfig();await refresh();await history();setInterval(refresh,2000);setInterval(history,5000);}catch(e){report(e);}})();</script></body></html>)AVUI";
static const char DEFAULT_CONFIG[] = R"AVCFG({
  "version": 1,
  "hardwareConfirmed": false,
  "busCommissioned": false,
  "species": "À définir",
  "ageDays": 0,
  "staleMs": 15000,
  "pollMs": 5000,
  "responseTimeoutMs": 1000,
  "interTransactionMs": 250,
  "sensors": {
    "ts": {
      "address": 1,
      "baud": 9600
    },
    "nh": {
      "address": 2,
      "baud": 9600,
      "model": "À confirmer",
      "modelConfirmed": false,
      "rangePpm": 500,
      "combined": false
    }
  },
  "profiles": [
    {
      "species": "À définir",
      "minAgeDays": 0,
      "maxAgeDays": 9999,
      "validated": false,
      "rules": {
        "hot": {
          "value": 30,
          "hysteresis": 1
        },
        "humid": {
          "value": 80,
          "hysteresis": 5
        },
        "co2": {
          "value": 1500,
          "hysteresis": 100
        },
        "nh3": {
          "value": 15,
          "hysteresis": 2
        },
        "cold": {
          "value": 20,
          "hysteresis": 1
        },
        "dry": {
          "value": 40,
          "hysteresis": 5
        },
        "minVentPeriodMs": 300000,
        "minVentOnMs": 60000
      }
    }
  ],
  "relays": [
    {
      "name": "Ventilation principale",
      "role": "vent_main",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "on",
      "sensorFault": "on",
      "observation": "off",
      "network": "continue_local"
    },
    {
      "name": "Ventilation complémentaire",
      "role": "vent_extra",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "off",
      "sensorFault": "on",
      "observation": "off",
      "network": "continue_local"
    },
    {
      "name": "Chauffage",
      "role": "heating",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "off",
      "sensorFault": "off",
      "observation": "off",
      "network": "continue_local"
    },
    {
      "name": "Humidification provisoire",
      "role": "humidifier",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "off",
      "sensorFault": "off",
      "observation": "off",
      "network": "continue_local"
    },
    {
      "name": "Éclairage",
      "role": "lighting",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "off",
      "sensorFault": "off",
      "observation": "off",
      "network": "continue_local"
    },
    {
      "name": "Alarme",
      "role": "alarm",
      "confirmed": false,
      "policyConfirmed": false,
      "minOnMs": 30000,
      "minOffMs": 30000,
      "startup": "on",
      "sensorFault": "on",
      "observation": "off",
      "network": "continue_local"
    }
  ]
}
)AVCFG";
