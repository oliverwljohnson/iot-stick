// A Simple application that interacts with the Spotify Web API

var SpotifyWebApi = require('spotify-web-api-node');

// Instantiating the new Spotify API (It automatically does the authentication process)
// Instantiation and Client Credentials authentication from thelinmichael/spotify-web-api-node examples
var clientId = '2722912b2a164140bc1ba1919913f5cd',
  clientSecret = '49929317ad3e4d0a87624f29525acd43';

// Create the api object with the credentials
var spotifyApi = new SpotifyWebApi({
  clientId: clientId,
  clientSecret: clientSecret
});

// Retrieve an access token.
function setCredentials(){
    console.log('Communicating with Spotify API');
    return spotifyApi.clientCredentialsGrant().then(
  function(data) {
    console.log('The access token expires in ' + data.body['expires_in']);
    console.log('The access token is ' + data.body['access_token']);

    // Save the access token so that it's used in future calls
    spotifyApi.setAccessToken(data.body['access_token']);
  },
  function(err) {
    console.log('Something went wrong when retrieving an access token', err);
  }
);
}

// Use of asynch function as all future API calls are dependent on the Auth key
// Want to find a better way of ensuring credentials are set before 
async function spotifyApiCalls(){
    await setCredentials();
    spotifyApi.getAvailableGenreSeeds().then(
        function(data) {
          console.log('Genre Seeds', data);
        },
        function(err) {
          console.error(err);
        }
      );
}


spotifyApiCalls();

// 1. Read in data from file (Perhaps it is better to implement it as a database with update function)
// 2. Process the data down into a seed object



// Get Reccomendations from Spotify Web API

//var cated = spotifyApi.getCategories().then();
//var seeds = spotifyApi.getAvailableGenreSeeds().then();
//console.log(seeds);


// Output Reccomendations