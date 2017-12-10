'use strict';

const fs = require('fs');

const ts = fs.readFileSync('testOutput.bin');

const freq_to_us = 4000;//000//000;//3914069;

//console.log(ts);

const dstTimes = [];
const srcTimes = [];
const timeDiff = [];

for(let n = 0; n < ts.length >>>6; n++) {
  const srcID = ts.readUIntBE((n << 6) + 8 + 8, 2);
  const dstID = ts.readUIntBE((n << 6) + 28 + 8 + 8, 2);
  const timeCount = ts.readUIntLE(n << 6, 6) / freq_to_us;
  if(srcID) {srcTimes[srcID] = timeCount; }
  if(dstID) {dstTimes[dstID] = timeCount; }
  //console.log(n, srcID, dstID, ts.readUIntLE(n << 6, 6), ts.slice((n << 6) + 8, (n << 6) + 64).toString('hex'));
}

for(let n = 0; n < 65536; n++) {
  timeDiff[n] = dstTimes[n] - srcTimes[n];
}

function numSort(a, b){
  if(a>b){
    return 1;
  }else if(a<b){
    return -1;
  } else {
    return 0;
  }
}

const filtered = timeDiff.filter((n)=> ((typeof n === 'number') && !isNaN(n)));
console.log(filtered.length)
const sortedTimeDiff = filtered.sort(numSort);
fs.writeFileSync('filteredStats.json', JSON.stringify(sortedTimeDiff));

console.log({
  "50%": `${filtered[Math.floor((filtered.length - 1) * 0.50)]}us`,
  "75%": `${filtered[Math.floor((filtered.length - 1) * 0.75)]}us`,
  "80%": `${filtered[Math.floor((filtered.length - 1) * 0.80)]}us`,
  "90%": `${filtered[Math.floor((filtered.length - 1) * 0.90)]}us`,
  "95%": `${filtered[Math.floor((filtered.length - 1) * 0.95)]}us`,
  "97%": `${filtered[Math.floor((filtered.length - 1) * 0.97)]}us`,
  "98%": `${filtered[Math.floor((filtered.length - 1) * 0.98)]}us`,
  "99%": `${filtered[Math.floor((filtered.length - 1) * 0.99)]}us`,
  "99.9%": `${filtered[Math.floor((filtered.length - 1) * 0.999)]}us`,
  "99.99%": `${filtered[Math.floor((filtered.length - 1) * 0.9999)]}us`,
  "99.999%": `${filtered[Math.floor((filtered.length - 1) * 0.99999)]}us`
})