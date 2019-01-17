// A Simple application that interacts with the Spotify Web API
// 1. Using the Client Credentials Flow get an authentication token


const request = require("request");

var client_id = '2722912b2a164140bc1ba1919913f5cd'; // Application ID
var client_secret = '49929317ad3e4d0a87624f29525acd43'; // Application Secret (May need updating)
var auth_json = null; // Pre defined before call

const authOptions = {
    url: 'https://accounts.spotify.com/api/token',
    headers: {
        'Authorization' : 'Basic ' + (new Buffer.from(client_id + ':' + client_secret).toString('base64'))
    },
    form: {
        grant_type: 'client_credentials'
      },
    json: true
}
// The concat function is commutative between string objects so a new Buffer is needed
console.log('Basic ' + (client_id + ':' + client_secret).toString('base64'));
console.log('Basic ' + (new Buffer.from(client_id + ':' + client_secret).toString('base64')));

request.post(authOptions, function(err, res, body){
    console.log("Authentication sucessful? ... " + !err);
    if(!err){
        auth_json = body
        console.log(auth_json);
        var token = auth_json.access_token;
        console.log(token);
    }
});

console.log(auth_json);
 