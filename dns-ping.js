'use strict';

const dns = require('dns');
dns.setServers(['38.140.60.122']);

function resolve(hostname, rrtype = 'A') {
  const startTime = process.hrtime();
  dns.resolve(hostname, rrtype, (err, records)=> {
    const requestTime = process.hrtime(startTime);
    console.log({ms: requestTime[0] * 1000 + requestTime[1] * 0.000001, hostname, rrtype, err, records})
  });
}

setInterval(resolve.bind(null, 'archeofutur.us'), 1000);

