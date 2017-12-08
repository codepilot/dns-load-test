'use strict';

const util = require('util');
const fs = require('fs');

const wsAdd = fs.createWriteStream('add-addresses.cmd');
const wsDel = fs.createWriteStream('del-addresses.cmd');
const addCmds = [];
const delCmds = [];

addCmds.push('@echo off', 'echo Modify rt-n to add ip addresses');
delCmds.push('@echo off', 'echo Modify rt-n to del ip addresses');

for(let i = 0; i < 256; i+=5) {
  addCmds.push(`netsh interface ipv4 add address "rt-1" 10.5.0.${i + 0} 255.0.0.0`);
  addCmds.push(`netsh interface ipv4 add address "rt-2" 10.5.0.${i + 1} 255.0.0.0`);
  addCmds.push(`netsh interface ipv4 add address "rt-3" 10.5.0.${i + 2} 255.0.0.0`);
  addCmds.push(`netsh interface ipv4 add address "rt-4" 10.5.0.${i + 3} 255.0.0.0`);
  addCmds.push(`netsh interface ipv4 add address "rt-5" 10.5.0.${i + 4} 255.0.0.0`);
  
  delCmds.push(`netsh interface ipv4 delete address "rt-1" 10.5.0.${i + 0} 255.0.0.0`);
  delCmds.push(`netsh interface ipv4 delete address "rt-2" 10.5.0.${i + 1} 255.0.0.0`);
  delCmds.push(`netsh interface ipv4 delete address "rt-3" 10.5.0.${i + 2} 255.0.0.0`);
  delCmds.push(`netsh interface ipv4 delete address "rt-4" 10.5.0.${i + 3} 255.0.0.0`);
  delCmds.push(`netsh interface ipv4 delete address "rt-5" 10.5.0.${i + 4} 255.0.0.0`);
}

addCmds.push(`pause`);
delCmds.push(`pause`);

wsAdd.end(addCmds.join('\r\n'));
wsDel.end(delCmds.join('\r\n'));
