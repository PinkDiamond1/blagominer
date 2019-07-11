var crypto = require('crypto');

var secret = Buffer.from('DISKCOIN641159  ');
var gensig0 = Buffer.from('ed377f7ea5f857c565baa9e44aa94f5c','hex');
var gensig1 = Buffer.from('8162c2fdf430d6eccdac4b856c6b5b68','hex');

var decipher0 = crypto.createDecipheriv('aes-128-cbc', secret, Buffer.alloc(16,0));
decipher0.setAutoPadding(false);
var newsig0 = decipher0.update(gensig0, 'buffer', 'hex');
var trash0 = decipher0.final('hex');

var decipher1 = crypto.createDecipheriv('aes-128-cbc', secret, Buffer.alloc(16,0));
decipher1.setAutoPadding(false);
var newsig1 = decipher1.update(gensig1, 'buffer', 'hex');
var trash1 = decipher1.final('hex'); 

// assert trash0 == ''
// assert trash1 == ''

var newsig = newsig0 + newsig1;
