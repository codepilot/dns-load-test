'use strict';

const child_process = require('child_process');
const util = require('util');
const fs = require('fs');

const cp_exec = util.promisify(child_process.exec);
const cp_execFile = util.promisify(child_process.execFile);
const fs_writeFile = util.promisify(fs.writeFile);
const fs_unlink = util.promisify(fs.unlink);

function execLoadTestSync({file, args, options}) {
  return child_process.execFileSync(file, args, options);
}

async function execLoadTest({file, params, options}) {
  return await cp_execFile(file, Object.entries(params).map(([k, v])=>`${k}=${v}`), options);
}


async function execLoadTests() {
  for(let clientIpRange = 24; clientIpRange <= 32; clientIpRange++) {
    for(let numCQs = 1; numCQs <= 1; numCQs++) {
      const testParams = {
        server: `10.2.0.4/32:53`,
        client: `10.5.0.0/${clientIpRange}`,
        SendPerAddress: 1,
        GlobalSendBufferSize: 65536,
        RunTimeSecond: 5,
        NumSendCQs: numCQs,
        NumReceiveCQs: numCQs};

      const execResult = await execLoadTest({
        file: `.\\x64\\Release\\dns-load-test.exe`,
        params: testParams,
        options: {
          cwd: `.`,
          timeout: 20*1000,
          windowsHide: true
        }
      });

      const textFileName = `./logs/log_clientIpRange${clientIpRange}_numCQs_${numCQs}.txt`;
      const jsonFileName = `./logs/log_clientIpRange${clientIpRange}_numCQs_${numCQs}.json`;
      await fs_writeFile(textFileName,  execResult.stdout);
      try {
        await fs_writeFile(jsonFileName,  JSON.stringify(JSON.parse(execResult.stdout), null, 2));
        await fs_unlink(textFileName);
      } catch(err) {
        
      }
      break;
    }
  }
}

execLoadTests();