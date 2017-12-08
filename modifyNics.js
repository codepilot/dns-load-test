'use strict';

const util = require('util');
const fs = require('fs');

const ws = fs.createWriteStream('add-addresses.cmd');

const cmds = [];

cmds.push('@echo off', 'echo Modify rt-3 to add ip addresses');

for(let i = 0; i < 256; i++) {
  cmds.push(`netsh interface ipv4 add address "rt-3" 10.5.0.${i} 255.0.0.0`);
}

cmds.push(`pause`);

ws.write(cmds.join('\r\n'));
ws.end();