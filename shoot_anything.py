# I'LL SHOOT ANYTHING
# The shooter game where you can shoot literally anything
# https://twitter.com/LanceCoyote
#
# Use the arrow keys to move and press space to fire
#
# This only works on Windows at the moment, sorry guys
# It's also really terrible
#
# Requires colorama and win32api to run

import colorama
import copy
import os
import random
import sys
import time
import win32api
import win32con


FRAME_RATE_LOCK   = 0.033   # Minimum delta time before advancing to the next frame
X_MAX             = 80      # Maximum x position
READ_MAX          = 255     # How many bytes do we read from each file?

advance_rate      = 0.8     # Initial advance rate of enemies, make this lower to make them faster

enemy_list        = []      # List of enemies
object_list       = []      # List of all objects
player_score      = 0

# Really basic 2D vector class
class Vector2 () :
  def __add__( self, other ) :
    if isinstance( other, Vector2 ) :
      # Adding vectors
      self.x += other.x
      self.y += other.y
    else :
      return NotImplemented
    return self

  def __eq__( self, other ) :
    if isinstance( other, Vector2 ) :
      # Comparing vectors
      if self.x == other.x and self.y == other.y :
        return True
    else :
      return NotImplemented
    return False

  def __init__( self, x, y ) :
    self.x = x
    self.y = y

# Top level game object class
class GameObject () :
  def kill( self ) :
    # When killed, remove this object from the object list
    if self in object_list :
      object_list.remove( self )

  def update( self, dt ) :
    pass

  def __init__( self, pos, char ) :
    self.pos    = pos
    self.char   = char

    object_list.append( self )

# Player ship class
class Ship ( GameObject ) :
  def move( self, distance ) :
    self.pos += distance

    # Don't let the player move past the top of the screen
    if self.pos.y < 1 :
      self.pos.y = 1

  def shoot( self ) :
    # Don't create a new bullet if one already exists
    if self.bullet :
      return

    self.bullet = Bullet( self )

  def update( self, dt ) :
    # Oh no! We're hit!
    for enemy in enemy_list :
      if self.pos == enemy.pos :
        self.kill()

    # Move up, move down
    if get_key( win32con.VK_UP ) :
      self.move( Vector2( 0, -1 ) )
    elif get_key( win32con.VK_DOWN ) :
      self.move( Vector2( 0, 1 ) )

    # Shoot lazers
    if get_key( win32con.VK_SPACE ) :
      self.shoot()

  def __init__( self ) :
    self.bullet = None

    GameObject.__init__( self, Vector2( 3, 10 ), '>' )

# Bullet class
class Bullet ( GameObject ) :
  def update( self, dt ) :
    global player_score

    # Check for collision
    for enemy in enemy_list :
      if self.pos == enemy.pos :
        # We hit an enemy, time to explode!
        player_score += ord( enemy.char ) * 15
        enemy.kill()
        self.player.bullet = None
        self.kill()
    else :
      # No collision, advance
      self.pos.x += 1

    if self.pos.x > X_MAX :
      # If we move past the end of the screen, die
      self.player.bullet = None
      self.kill()

  def __init__( self, player ) :
    self.player = player

    GameObject.__init__( self, copy.copy( player.pos ) + Vector2( 1, 0 ), '~' )

# Enemy class
class Enemy ( GameObject ) :
  def update( self, dt ) :
    self.wait_time += dt

    # If we're supposed to advance, then advance!
    if self.wait_time > advance_rate :
      self.pos.x -= 1
      self.wait_time = 0

    # Once we move past the left edge of the screen, die
    if self.pos.x < 1 :
      self.kill()

  def kill( self ) :
    # Remove ourselves from the enemy list when we die
    if self in enemy_list :
      enemy_list.remove( self )

    GameObject.kill( self )

  def __init__( self, pos, char ) :
    self.wait_time = 0

    GameObject.__init__( self, pos, char )

    enemy_list.append( self )


# File interface functions
def get_random_file( ) :
  f         = ""
  p         = os.getcwd()
  current_p = ""

  # Search through directories recursively until we find a valid file
  while not os.path.isfile( f ) :
    filelist = os.listdir( p )

    if filelist :
      f = random.choice( filelist )

      # Not a file, keep looking!
      if not os.path.isfile( f ) :
        p += "\\" + f
        current_p += "\\" + f
    else :
      return get_random_file( )

  if current_p :
    current_p += "\\"
  return current_p[1:] + f

# Game logic and rendering functions
def draw_intro() :
  print("\033[2J")

  print("\033[7;30HI'LL SHOOT ANYTHING!")
  print("\033[9;10HThere's too many files in the repo - take 'em out, space ace")
  print("\033[10;15HPress [UP] and [DOWN] to move, use [SPACE] to fire")
  print("\033[12;21HHit [SPACE] to start or [ESC] to bail")

def draw_game( dt ) :
  print("\033[2J")  # Clear screen
  print("SCORE: {:0>8}".format( player_score ) )

  for obj in object_list :
    # Don't draw objects off screen
    if obj.pos.x > X_MAX :
      continue

    # Show the object
    print("\033[{};{}H{}".format( obj.pos.y, obj.pos.x, obj.char ) )

  sys.stdout.flush()

def draw_gameover() :
  print("\033[2J")

  print("\033[10;35HGAME OVER!")
  print("\033[11;21HPress [SPACE] to restart or [ESC] to chicken out")

def gameloop( dt ) :
  global advance_rate
  global player_score

  # If all the enemies are dead, spawn a new wave and speed up
  if len( enemy_list ) == 0 :
    player_score += 100000
    advance_rate *= 0.75
    generate_new_wave()

  # Update every object
  for obj in object_list :
    obj.update( dt )

def generate_new_wave() :
  # Grab and open a random file
  with open( get_random_file() ) as f :
    data = f.read( READ_MAX )
    x, y = X_MAX, 5

    # Loop through our data
    for char in data :
      if char.isspace() :
        # Whitespace, don't create an enemy
        if char == "\n" :
          x = X_MAX
          y += 1
      else :
        # It's an enemy! Run!
        Enemy( Vector2( x, y ), char )
      x += 1

def get_key( key ) :
  return win32api.GetAsyncKeyState( key )


# Program entry
def main() :
  global object_list, enemy_list, player_score, advance_rate

  colorama.init()

  keep_running  = True
  dt            = FRAME_RATE_LOCK

  draw_intro()
  while not get_key( win32con.VK_SPACE ) :
    if get_key( win32con.VK_ESCAPE ) :
      keep_running = False
      break

  while keep_running :
    player_score  = 0
    advance_rate  = 0

    player        = Ship( )
    generate_new_wave()

    while player in object_list :
      # Capture the frame start time
      frame_start = time.time()

      # Run a game loop
      gameloop( dt )
      draw_game( dt )

      # Delay our next frame if we rendered this one too fast
      if time.time() - frame_start < FRAME_RATE_LOCK :
        time.sleep( FRAME_RATE_LOCK - ( time.time() - frame_start ) )

      # Set deltatime
      dt = time.time() - frame_start

    object_list = []
    enemy_list  = []

    draw_gameover()
    while not get_key( win32con.VK_SPACE ) :
      if get_key( win32con.VK_ESCAPE ) :
        keep_running = False
        break

if __name__ == "__main__" :
  main()
