import spotipy
import spotipy.util as util
import json
import datetime
import sched, time
import subprocess
import getpass
import serial_read


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
else:
    print("Auth failed for", username)

me = spotify.me()

# Check that "Pi Radio" is setup
def checkEnvironment():
    devices = spotify.devices()
    print(devices)
    if len(devices) > 0:
        print("There are", len(devices['devices']), "active players")
    else:
        print("There are no active players, beginning local player")
        subprocess.Popen(["librespot","-n",player_name,"-u",username,"-p",getpass.getpass()])

checkEnvironment()

# Setup Playing Context
playlist = spotify.user_playlist_create(user=me['id'], name=("IoT Recommends - "+datetime.datetime.now().strftime('%c')), description=("IoT Recommends"))
print("The current context is", playlist["uri"])

# Suggest Songs from reccomendations


# getNextSong()
spotify.start_playback(uris=['spotify:track:2374M0fQpWi3dLnB54qaLX'])
# Queue Songs, Forcing them to play


def getSuggestions():
    serial_read.readUART()
    f = open("reccomendations", "r")
    genres_list = text_file.read().split(',')
    print(f.readline(10))
    print("Of the Availiable genres:", spotify.recommendation_genre_seeds(), "\n the following seeds were given:", genres)
    # TODO: Implement the file format and thus parsing code
    reccomendationsObject = spotify.recommendations(seed_genres=genres,limit=1,country="AU")
    return reccomendationsObject

def playSuggestion(suggested,playlist):
    print([suggested])
    spotify.user_playlist_add_tracks(user=me['id'], playlist_id=playlist['id'], tracks=[suggested['tracks'][0]['uri']])
    spotify.start_playback(context_uri=playlist['uri'])
    return True

# Sets the context for playing by writing a new playlist





suggested = getSuggestions()
# print(json.dumps(playlist, indent = 2))
# playSuggestion(suggested,playlist)

s = sched.scheduler(time.time, time.sleep)
def play_loop(sc): 
    print("Playing loop...")
    currentTrack = spotify.current_user_playing_track()
    if((currentTrack['item']['duration_ms'] - currentTrack['progress_ms']) < (15 * 1000)):
        print("Reading Network File...")
        suggested = getSuggestions()
        spotify.user_playlist_add_tracks(user=me['id'], playlist_id=playlist['id'], tracks=[suggested['tracks'][0]['uri']])
    s.enter(5, 1, play_loop, (sc,))

# s.enter(5, 1, play_loop, (s,))
# s.run()

