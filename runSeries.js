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

const MaxSendCount=5000000
const totalOutstandingRequests = 16384;
const NumSendCQs = 4;
const NumReceiveCQs = 4;
const RunTimeSecond = 5;
const server = `10.2.0.4/32:53`;

async function execLoadTests() {
  const batchFile = [];
  for(let clientIpRange = 32; clientIpRange >= 24; clientIpRange--) {
    const clientIpCount = Math.pow(2, 32 - clientIpRange);
    const SendPerAddress = totalOutstandingRequests / clientIpCount;
    const execFile = `.\\x64\\Release\\dns-load-test.exe`;

    {
      const testParams = {
        server,
        client: `10.5.0.0/${clientIpRange}`,
        SendPerAddress,
        RunTimeSecond,
        NumSendCQs,
        NumReceiveCQs,
        MaxSendCount};

      const textFileName = `./logs/log_clientIpRange${clientIpRange}.txt`;
      const jsonFileName = `./logs/log_clientIpRange${clientIpRange}.json`;

      batchFile.push(`"${execFile}" ${Object.entries(testParams).map(([k, v])=>`${k}=${v}`).join(' ')} > ${jsonFileName}`);
      
      continue;
      const execResult = await execLoadTest({
        file: execFile,
        params: testParams,
        options: {
          cwd: `.`,
          timeout: 20*1000,
          windowsHide: true,
          maxBuffer: 2 * 1024 * 1024
        }
      });

      await fs_writeFile(textFileName,  execResult.stdout);
      try {
        await fs_writeFile(jsonFileName,  JSON.stringify(JSON.parse(execResult.stdout), null, 2));
        await fs_unlink(textFileName);
      } catch(err) {
        
      }
    }
  }
  fs.writeFileSync('runSeries.cmd', batchFile.join('\r\n'));
}

execLoadTests();