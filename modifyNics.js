'use strict';

const util = require('util');
const fs = require('fs');

const wsAddv4 = fs.createWriteStream('add-addresses-v4.cmd');
const wsAddv6 = fs.createWriteStream('add-addresses-v6.cmd');
const wsDelv4 = fs.createWriteStream('del-addresses-v4.cmd');
const wsDelv6 = fs.createWriteStream('del-addresses-v6.cmd');
const addCmdsv4 = [];
const addCmdsv6 = [];
const delCmdsv4 = [];
const delCmdsv6 = [];

addCmdsv4.push('@echo off', 'echo Modify rt-n to add ipv4 addresses');
addCmdsv6.push('@echo off', 'echo Modify rt-n to add ipv6 addresses');
delCmdsv4.push('@echo off', 'echo Modify rt-n to del ipv4 addresses');
delCmdsv6.push('@echo off', 'echo Modify rt-n to del ipv6 addresses');

const numNics = 4;

for(let i = 0; i < 256; i+=numNics) {
  for(let n = 0; n < numNics && i + n < 256; n++) {
    addCmdsv4.push(`netsh interface ipv4 add    address "SFP${1 + n}" 10.5.0.${i + n} 255.0.0.0`);
    delCmdsv4.push(`netsh interface ipv4 delete address "SFP${1 + n}" 10.5.0.${i + n} 255.0.0.0`);
    addCmdsv6.push(`netsh interface ipv6 add    address "SFP${1 + n}" fd00::5:0:${(i + n).toString(16)}/8`);
    delCmdsv6.push(`netsh interface ipv6 delete address "SFP${1 + n}" fd00::5:0:${(i + n).toString(16)}`);
  }
}

addCmdsv4.push(`pause`);
addCmdsv6.push(`pause`);
delCmdsv4.push(`pause`);
delCmdsv6.push(`pause`);

wsAddv4.end(addCmdsv4.join('\r\n'));
wsAddv6.end(addCmdsv6.join('\r\n'));
wsDelv4.end(delCmdsv4.join('\r\n'));
wsDelv6.end(delCmdsv6.join('\r\n'));
