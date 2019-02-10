# -*- coding: utf-8 -*-
"""
Created on Sat Feb  9 21:04:42 2019

@author: Oliver Johnson
"""

import spotipy
import spotipy.util as util
import json

SPOTIPY_CLIENT_ID ='2722912b2a164140bc1ba1919913f5cd'
SPOTIPY_CLIENT_SECRET ='49929317ad3e4d0a87624f29525acd43'
SPOTIPY_REDIRECT_URI ='http://localhost:8888'
REFRESH_TOKEN = 'AQCzdqLIGDRJKyZdGybmxW2Ba9tg1OE0SL1gvLRHwNTxwdzFmybQt3rc8jB-NmplleDs583k2BmwoH2NwvMTtHC_gev54D6a0tB7kK6Sl9LCW4G7NeezXjCNpPvP6NEK-bgiLw'
scope = 'user-read-private user-read-email playlist-modify-private playlist-modify-public playlist-read-collaborative user-modify-playback-state user-read-currently-playing user-read-playback-state user-top-read user-read-recently-played app-remote-control streaming user-read-email user-follow-read user-follow-modify user-library-modify user-library-read'
username = "Oliver.Johnson1998@gmail.com"
spotify = spotipy.Spotify


token = util.prompt_for_user_token(username,scope,client_id=SPOTIPY_CLIENT_ID,client_secret=SPOTIPY_CLIENT_SECRET,redirect_uri=SPOTIPY_REDIRECT_URI)
if token:
    spotify = spotipy.Spotify(auth=token)
    print("Initialization Complete, auth approved for", username, "for scopes:", scope)
else:
    print("Auth failed for", username)


# resuts = client.Spotify.current_user_saved_tracks(spotify)
# spotify.trace = False
def getSuggestions():
    reccomendFile = open("reccomendations", "r")
    genres = ["rock"]
    print("Of the Availiable genres:", spotify.recommendation_genre_seeds(), "\n the following seeds were given:", genres)
    # TODO: Implement the file format and thus parsing code
    reccomendationsObject = spotify.recommendations(seed_genres=genres,limit=1,country="AU")
    return reccomendationsObject

def playSuggestion():
    return True

suggested = getSuggestions()
print(json.dumps(suggested,indent=3))
print(suggested["tracks"][0]["name"])






# Read the options from file
# Get suggestions
# Check suggestions against songs played for the last 2 hours
#   If song is new then play
#   else get reccomendations changing




