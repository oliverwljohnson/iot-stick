import spotipy
import spotipy.util as util
import json
import datetime
import sched, time
import subprocess
import getpass
import serial_read
from collections import Counter
import random


SPOTIPY_CLIENT_ID ='2722912b2a164140bc1ba1919913f5cd'
SPOTIPY_CLIENT_SECRET ='49929317ad3e4d0a87624f29525acd43'
SPOTIPY_REDIRECT_URI ='http://localhost:8888'
REFRESH_TOKEN = 'AQCzdqLIGDRJKyZdGybmxW2Ba9tg1OE0SL1gvLRHwNTxwdzFmybQt3rc8jB-NmplleDs583k2BmwoH2NwvMTtHC_gev54D6a0tB7kK6Sl9LCW4G7NeezXjCNpPvP6NEK-bgiLw'
scope = 'user-read-private user-read-email playlist-modify-private playlist-modify-public playlist-read-collaborative user-modify-playback-state user-read-currently-playing user-read-playback-state user-top-read user-read-recently-played app-remote-control streaming user-read-email user-follow-read user-follow-modify user-library-modify user-library-read'
username = "oliver.johnson1998@gmail.com"
spotify = spotipy.Spotify
player_name = "Pi Radio"


token = util.prompt_for_user_token(username,scope,client_id=SPOTIPY_CLIENT_ID,client_secret=SPOTIPY_CLIENT_SECRET,redirect_uri=SPOTIPY_REDIRECT_URI)
if token:
    spotify = spotipy.Spotify(auth=token)
    print("Initialization Complete, auth approved for", username, "for scopes:", scope)
    me = spotify.me()
else:
    print("Auth failed for", username)


# Check that "Pi Radio" is setup
def checkEnvironment():
    devices = spotify.devices()
    print(devices)
    if len(devices) > 0:
        print("There are", len(devices['devices']), "devices")

# A Vote Selection algorithm
def getVotes_a_la_mode(votes):
    vote_count = Counter(votes)
    top_count = vote_count.most_common(5)
    if top_count == []:
        return ['pop']
    else:
        empty = []
        for pair in top_count:
            empty.append(pair[0])
    return empty

def getVotes_a_la_random(votes):
    if votes == []:
        return ['pop']
    else:
        empty = []
        for x in range(5):
            empty.append(votes[random.randint(0,len(votes)-1)])
    return empty

# Get suggestion from votes
def getSuggestions():
    serial_read.readUART()
    f = open("reccomendations", "r")
    g_time, *g_votes = f.readline().strip().split(',')
    print("Votes taken at",time.ctime(int(g_time)))
    print("The votes are:",g_votes)
    genres = getVotes_a_la_random(g_votes)
    print("Of the Availiable genres:", spotify.recommendation_genre_seeds(), "\n We will be listening to:",genres)
    reccomendationsObject = spotify.recommendations(seed_genres=genres,limit=1,country="AU")
    print(json.dumps(reccomendationsObject,indent=2))
    return reccomendationsObject

def queueSong(sugg):
    spotify.user_playlist_add_tracks(user=me['id'], playlist_id=playlist['id'], tracks=[sugg['tracks'][0]['uri']])
    return sugg['tracks'][0]['uri']

def playSong():
    spotify.start_playback(context_uri=playlist['uri'])





checkEnvironment()
playlist = spotify.user_playlist_create(user=me['id'], name=("IoT Recommends - "+datetime.datetime.now().strftime('%c')), description=("IoT Recommends"))
print("The current context is", playlist["uri"])

latestTrack = queueSong(getSuggestions())
playSong()

s = sched.scheduler(time.time, time.sleep)
def play_loop(sc): 
    print("Playing loop...")
    currentTrack = spotify.current_user_playing_track()
    currentContext = spotify.current_playback()['context']
    print(json.dumps(currentContext, indent=2)) 
    if currentContext is None:
        spotify.start_playback(context_uri=playlist['uri'],offset={"uri":latestTrack})
    if((currentTrack['item']['duration_ms'] - currentTrack['progress_ms']) < (30 * 1000)):
        print("Reading Network File...")
        global latestTrack
        latestTrack = queueSong(getSuggestions())
        print(latestTrack)
    s.enter(5, 1, play_loop, (sc,))

s.enter(5, 1, play_loop, (s,))
s.run()

