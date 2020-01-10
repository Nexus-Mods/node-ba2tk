Object.defineProperty(exports, "__esModule", { value: true });

let nbind = require('nbind');
let path = require('path');

let ba2tk;
try {
  ba2tk = nbind.init(path.join(__dirname, 'ba2tk')).lib;
} catch (err) {
  if (err.message.indexOf('Could not locate the bindings file') !== -1) {
    ba2tk = nbind.init().lib;
  } else {
    throw err;
  }
}

module.exports.default = ba2tk.loadBA2;
