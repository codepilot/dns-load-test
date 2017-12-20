'use strict';

const dns = require('dns');
dns.setServers(['38.140.60.122']);

function resolve(hostname, rrtype = 'A') {
  dns.resolve(hostname, rrtype, (err, records)=> console.log({hostname, rrtype, err, records}));
}

resolve('archeofutur.us', 'A');
resolve('www.archeofutur.us', 'A');

resolve('raw1.archeofutur.us', 'A');
resolve('raw2.archeofutur.us', 'A');
resolve('raw3.archeofutur.us', 'A');
resolve('raw4.archeofutur.us', 'A');
resolve('raw5.archeofutur.us', 'A');
resolve('raw6.archeofutur.us', 'A');
resolve('raw7.archeofutur.us', 'A');
resolve('raw8.archeofutur.us', 'A');

resolve('ns1.archeofutur.us', 'A');
resolve('ns2.archeofutur.us', 'A');
resolve('ns3.archeofutur.us', 'A');
resolve('ns4.archeofutur.us', 'A');


resolve('archeofutur.us', 'AAAA');
resolve('www.archeofutur.us', 'AAAA');

resolve('raw1.archeofutur.us', 'AAAA');
resolve('raw2.archeofutur.us', 'AAAA');
resolve('raw3.archeofutur.us', 'AAAA');
resolve('raw4.archeofutur.us', 'AAAA');
resolve('raw5.archeofutur.us', 'AAAA');
resolve('raw6.archeofutur.us', 'AAAA');
resolve('raw7.archeofutur.us', 'AAAA');
resolve('raw8.archeofutur.us', 'AAAA');

resolve('ns1.archeofutur.us', 'AAAA');
resolve('ns2.archeofutur.us', 'AAAA');
resolve('ns3.archeofutur.us', 'AAAA');
resolve('ns4.archeofutur.us', 'AAAA');

