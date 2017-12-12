'use strict';

const fs = require('fs');

const freq_to_us = 4000;//000//000; 4GHz base clock, remove 6 zeros for Microseconds

const dstTimes = [];
const srcTimes = [];
const timeDiffOld = [];
const timeDiff = [];

const rfs = fs.openSync('testOutput.bin', 'r');

let filePos = 0;
let maxFileSize = 1602486272; //bigger runs out of memory
while(filePos < maxFileSize) {
  let ts = Buffer.alloc(1024*1024);
  ts.slice(0, fs.readSync(rfs, ts, 0, ts.length, filePos));
  filePos += ts.length;
  if(!ts.length) { break; }
  for(let n = 0; n < ts.length >>>6; n++) {
    const srcID = ts.readUIntBE((n << 6) + 8 + 8, 2);
    const dstID = ts.readUIntBE((n << 6) + 28 + 8 + 8, 2);
    const timeCount = ts.readUIntLE(n << 6, 6) / freq_to_us;
    if(srcID) {srcTimes[srcID] = timeCount; }
    if(dstID) {
      if(typeof srcTimes[dstID] === 'number') {
        timeDiff.push(timeCount - srcTimes[dstID]);
        delete srcTimes[dstID];
      } else {
        dstTimes[dstID] = timeCount;
      }
    }
  }

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
fs.writeFileSync('filteredStats.json', JSON.stringify(filtered));
const sortedTimeDiff = filtered.sort(numSort);
fs.writeFileSync('sortedFilteredStats.json', JSON.stringify(sortedTimeDiff));

const timeSum = sortedTimeDiff.reduce((p, c, i, a)=> p + c, 0);
const timeAverage = timeSum / filtered.length;
const variance = sortedTimeDiff.reduce((p, c, i, a)=> p + Math.pow(c - timeAverage, 2), 0);
const varianceAverage = variance / filtered.length;
const standardDeviation = Math.sqrt(varianceAverage);

console.log({
  "Average": `${timeAverage}us`,
  "StandardDeviation": `${standardDeviation}us`,
  "0%": `${filtered[0]}us`,
  "25%": `${filtered[Math.floor((filtered.length - 1) * 0.25)]}us`,
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