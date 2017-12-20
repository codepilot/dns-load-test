'use strict';

const { Resolver } = require('dns');
const resolver = new Resolver();

resolver.setServers(['38.140.60.122']);

function resolve(hostname, rrtype = 'A') {
  resolver.cancel();
  const startTime = process.hrtime();
  resolver.resolve(hostname, rrtype, (err, records)=> {
    const requestTime = process.hrtime(startTime);
    console.log({ms: requestTime[0] * 1000 + requestTime[1] * 0.000001, hostname, rrtype, err, records})
  });
}

setInterval(resolve.bind(null, 'archeofutur.us'), 1000);

