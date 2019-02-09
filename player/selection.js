var SpotifyWebApi = require('spotify-web-api-node');
var jsonQuery = require('json-query');

// Variables for User Authorization
var scopes = ['user-read-private', 'user-read-email'],
  redirectUri = 'http://localhost:8888/callback',
  clientId = '2722912b2a164140bc1ba1919913f5cd',
  clientSecret = '49929317ad3e4d0a87624f29525acd43'
  state = 'true';

// Setting credentials can be done in the wrapper's constructor, or using the API object's setters.
var spotifyApi = new SpotifyWebApi({
  redirectUri: redirectUri,
  clientId: clientId,
  clientSecret: clientSecret
});

var devices = null;

/* Reattempts @function f after first refreshing the access 
*/
async function retryAfterRefresh(f){
    await spotifyApi.refreshAccessToken().then(
        function(data) {
          console.log('The access token has been refreshed!');
      
          // Save the access token so that it's used in future calls
          spotifyApi.setAccessToken(data.body['access_token']);
        },
        function(err) {
          console.log('Could not refresh access token', err);
        });
    f();
}

spotifyApi.setRefreshToken('AQCzdqLIGDRJKyZdGybmxW2Ba9tg1OE0SL1gvLRHwNTxwdzFmybQt3rc8jB-NmplleDs583k2BmwoH2NwvMTtHC_gev54D6a0tB7kK6Sl9LCW4G7NeezXjCNpPvP6NEK-bgiLw');


// Get Device list, if device "Pi Radio" is availiable 
function getDevices() {
    spotifyApi.getMyDevices().then(
        function(data) {
            console.log('Here is a list of devices');
            console.log(data.body);
            return data;
        },
        function(err) {
            console.log('There was an error in getting your devices');
            console.log(err);
            retryAfterRefresh(getDevices);
        });
}


async function init(){
    retryAfterRefresh(console.log);
    let promise = new Promise (function(resolve, reject) {
        resolve(getDevices());
      });
    devices = await promise[0];
}

let promise = new Promise(function(resolve, reject) {
    resolve("done");
  
    reject(new Error("…")); // ignored
    setTimeout(() => resolve("…")); // ignored
  });

init();