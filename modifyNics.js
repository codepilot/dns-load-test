'use strict';

const util = require('util');
const fs = require('fs');

const wsAdd = fs.createWriteStream('add-addresses.cmd');
const wsDel = fs.createWriteStream('del-addresses.cmd');
const addCmds = [];
const delCmds = [];

addCmds.push('@echo off', 'echo Modify rt-n to add ip addresses');
delCmds.push('@echo off', 'echo Modify rt-n to del ip addresses');

const numNics = 4;

for(let i = 0; i < 256; i+=numNics) {
  for(let n = 0; n < numNics && i + n < 256; n++) {
    addCmds.push(`netsh interface ipv4 add address "rt-${1 + n}" 10.5.0.${i + n} 255.0.0.0`);
    delCmds.push(`netsh interface ipv4 delete address "rt-${1 + n}" 10.5.0.${i + n} 255.0.0.0`);
  }
}

addCmds.push(`pause`);
delCmds.push(`pause`);

wsAdd.end(addCmds.join('\r\n'));
wsDel.end(delCmds.join('\r\n'));
