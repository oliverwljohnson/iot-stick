// Depends on node.js
console.warn("initialising test server");


// load http library
var http = require("http");
console.log("loading http library");


// create http server to handle responses
console.log("creating server");
http.createServer(function(request,response) {
    response.writeHead(200, {"Content-Type": "text/plain"});
    response.write("Hello World!");
    response.end();
}).listen(8888);