In 2013, the mobile game Flappy Bird took the world by storm. You’ll be developing a Q-learning agent to play a similar game, Swingy Monkey. In this game, you control a monkey that is trying to swing on vines and avoid tree trunks. You can either make him jump to a new vine, or have him swing down on the vine he’s currently holding. You get points for successfully passing tree trunks without hitting them, falling off the bottom of the screen, or jumping off the top. There are some sources of randomness: the monkey’s jumps are sometimes higher than others, the gaps in the trees vary vertically, the gravity varies from game to game, and the distances between the trees are different. You can play the game directly by pushing a key on the keyboard to make the monkey jump. However, your objective is to build an agent that learns to play on its own. 

Task: Your task is to use Q-learning to find a policy for the monkey that can navigate the trees. 

```python
class Learner(object):
    """
    Implement this bot! A good start is to look at the skeleton of RandomJumper, which
    frames the bot in a good way, but has poor learning logic.
    """

    def __init__(self):
        pass

    def discretize_state(self, state):
        """
        Discretize the position space to produce binned features.
        rel_x = the binned relative horizontal distance between the monkey and the tree
        rel_y = the binned relative vertical distance between the monkey and the tree
        """

        rel_x = int((state["tree"]["dist"]) // X_BINSIZE)
        rel_y = int((state["tree"]["top"] - state["monkey"]["top"]) // Y_BINSIZE)
        return (rel_x, rel_y)

    def action_callback(self, state):
        """
        Whenever the state changes, this function will be called.
        Return 0 if you don't want to jump and 1 if you do.
        """

        # 1. Discretize 'state' to get your transformed 'current state' features.
        dstate = self.discretize_state(state)

        # 2. Perform the Q-Learning update using 'current state' and the 'last state'.

        # 3. Choose the next action using an epsilon-greedy policy.
        pass

    def reward_callback(self, reward):
        """
        When you are given a reward, this function is called.
        """
        pass
```

At the beginning of the code, you will import the SwingyMonkey class, which is the implementation of the game that has already been completed for you. 

Additionally, we provide a video of the staff Q-Learner playing the game at https://youtu.be/xRD6xBQbauw. It figures out a reasonable policy in a few iterations. You’ll be responsible for implementing the Python function action callback. The action callback will take in a dictionary that describes the current state of the game and return an action for the next time step. This will be a binary action, where 0 means to swing downward and 1 means to jump up. The dictionary you get for the state looks like this:

{ score, tree: {dist <pixels to the next tree trunk>, top <height of the top of the tree trunk gap>, bottom <height of the bottom of the tree trunk gap>}, monkey: {vel <current monkey y-axis speed>, top <height of top of monkey>, bot <height of botom of monkey>}}

All of the units here (except score) will be in screen pixels. Note that since the state space is very large (effectively continuous), the monkey’s relative position needs to be discretized into bins. The pre-defined function discretize state does this for you.

Requirements: Code: First, you should implement Q-learning with an ε-greedy policy yourself. You can increase the performance by trying out different parameters for the learning rate α, discount rate γ, and exploration rate ε. Do not use outside RL code for this assignment. Second, you should use a method of your choice to further improve the performance. This could be inferring gravity at each epoch (the gravity varies from game to game), updating the reward function, trying decaying epsilon greedy functions, changing the features in the state space, and more. One of our staff solutions got scores over 800 before the 100th epoch, but you are only expected to reach scores over 50 before the 100th epoch.

Here is the code which runs the simulation:
```python
import matplotlib.image as mpimg
import numpy.random as npr


class SwingyMonkey:

    def __init__(self, sound=True, text=None, action_callback=None, 
                 reward_callback=None, tick_length=100):
        """Constructor for the SwingyMonkey class.

        Possible Keyword Arguments:

        sound: Boolean variable on whether or not to play sounds.
               Defaults to True.

        text: Optional string to display in the upper right corner of
              the screen.

        action_callback: Function handle for determining actions.
                         Takes a dictionary as an argument.  The
                         dictionary contains the current state of the
                         game.

        reward_callback: Function handle for receiving rewards. Takes
                         a scalar argument which is the reward.

        tick_length: Time in milliseconds between game steps.
                     Defaults to 100ms, but you might want to make it
                     smaller for training."""

        # Don't change these!!!
        self.screen_width  = 600
        self.screen_height = 400
        self.horz_speed    = 25
        self.impulse       = 15
        self.gravity       = npr.choice([1,4])
        self.tree_mean     = 5
        self.tree_gap      = 200
        self.tree_offset   = -300
        self.edge_penalty  = -10.0
        self.tree_penalty  = -5.0
        self.tree_reward   = 1.0

        # Store arguments.
        self.sound         = sound
        self.action_fn     = action_callback
        self.reward_fn     = reward_callback
        self.tick_length   = tick_length
        self.text          = text

        # # Initialize pygame.
        # pg.init()
        # try:
        #     pg.mixer.init()
        # except:
        #     print("No sound.")
        #     self.sound = False

        # Set up the screen for rendering.
        # self.screen = pg.display.set_mode((self.screen_width, self.screen_height), 0, 32)

        # # Load external resources.
        # self.background_img = pg.image.load('res/jungle-pixel.bmp').convert()
        self.monkey_img = mpimg.imread('./res/monkey.bmp')
        self.tree_img = mpimg.imread('./res/tree-pixel.bmp')
        # if self.sound:
        #     self.screech_snd    = pg.mixer.Sound('res/screech.wav')
        #     self.blop_snd       = pg.mixer.Sound('res/blop.wav')

        # # Set up text rendering.
        # self.font = pg.font.Font(None, 36)

        # Track locations of trees and gaps.
        self.trees     = []
        self.next_tree = 0
        
        # Precompute some things about the monkey.
        self.monkey_left  = self.screen_width/2 - self.monkey_img.shape[1]/2
        self.monkey_right = self.monkey_left + self.monkey_img.shape[1]
        self.monkey_loc   = self.screen_height/2 - self.monkey_img.shape[0]/2

        # Track game state.
        self.vel   = 0
        self.hook  = self.screen_width
        self.score = 0
        self.iter  = 0

    def get_state(self):
        '''Returns a snapshot of the current game state, computed
        relative to to the next oncoming tree.  This is a dictionary
        with the following structure:
        { 'score': <current score>,
          'tree': { 'dist': <pixels to next tree trunk>,
                    'top':  <screen height of top of tree trunk gap>,
                    'bot':  <screen height of bottom of tree trunk gap> },
          'monkey': { 'vel': <current monkey y-axis speed in pixels per iteration>,
                      'top': <screen height of top of monkey>,
                      'bot': <screen height of bottom of monkey> }}'''                      

        # Find the next closest tree.
        next_tree = None
        for tree in self.trees:
            if tree['x']+290 >= self.monkey_left:
                next_tree = tree.copy()
                break

        if not next_tree:
            next_tree = self.trees[0].copy()

        # Construct the state dictionary to return.
        return { 'score': self.score,
                 'tree': { 'dist': next_tree['x']+215-self.monkey_right,
                           'top': self.screen_height-next_tree['y'],
                           'bot': self.screen_height-next_tree['y']-self.tree_gap},
                 'monkey': { 'vel': self.vel,
                             'top': self.screen_height - self.monkey_loc + self.monkey_img.shape[0]/2,
                             'bot': self.screen_height - self.monkey_loc - self.monkey_img.shape[0]/2}}

    def game_loop(self):
        '''This is called every game tick.  You call this in a loop
        until it returns false, which means you hit a tree trunk, fell
        off the bottom of the screen, or jumped off the top of the
        screen.  It calls the action and reward callbacks.'''

        # Render the background.
        # self.screen.blit(self.background_img, (self.iter,0))
        # if self.iter < self.background_img.shape[1] - self.screen_width:
        #     self.screen.blit(self.background_img, (self.iter+self.background_img.shape[1],0))

        # Perhaps generate a new tree.
        if self.next_tree <= 0:
            self.next_tree = self.tree_img.shape[1] * 5 + int(npr.geometric(1.0/self.tree_mean))
            self.trees.append( { 'x': self.screen_width+1,
                                 'y': int((0.3 + npr.rand()*0.65)*(self.screen_height-self.tree_gap)),
                                 's': False })
        # # Process input events.
        # for event in pg.event.get():
        #     if event.type == pg.QUIT:
        #         sys.exit()
        #     elif self.action_fn is None and event.type == pg.KEYDOWN:
        #         self.vel = npr.poisson(self.impulse)
        #         self.hook = self.screen_width

        # Perhaps take an action via the callback.
        if self.action_fn is not None and self.action_fn(self.get_state()):
            self.vel = npr.poisson(self.impulse)
            # self.hook = self.screen_width

        # Eliminate trees that have moved off the screen.
        self.trees = [x for x in self.trees if x['x'] > -self.tree_img.shape[1]]

        # Monkey dynamics
        self.monkey_loc -= self.vel
        self.vel        -= self.gravity

        # Current monkey bounds.
        monkey_top = self.monkey_loc - self.monkey_img.shape[0]/2
        monkey_bot = self.monkey_loc + self.monkey_img.shape[0]/2

        # Move trees to the left, render and compute collision.
        self.next_tree -= self.horz_speed
        edge_hit = False
        tree_hit = False
        pass_tree = False
        for tree in self.trees:
            tree['x'] -= self.horz_speed

            # # Render tree.
            # self.screen.blit(self.tree_img, (tree['x'], self.tree_offset))

            # # Render gap in tree.
            # self.screen.blit(self.background_img, (tree['x'], tree['y']),
            #                  (tree['x']-self.iter, tree['y'],
            #                   self.tree_img.shape[1], self.tree_gap))
            # if self.iter < self.background_img.shape[1] - self.screen_width:
            #     self.screen.blit(self.background_img, (tree['x'], tree['y']),
            #                      (tree['x']-(self.iter+self.background_img.shape[1]), tree['y'],
            #                       self.tree_img.shape[1], self.tree_gap))
                
            trunk_left  = tree['x']
            trunk_right = tree['x'] + self.tree_img.shape[1]
            trunk_top   = tree['y']
            trunk_bot   = tree['y'] + self.tree_gap

            # Compute collision.
            if (((trunk_left < (self.monkey_left+15)) and (trunk_right > (self.monkey_left+15))) or
                ((trunk_left < self.monkey_right) and (trunk_right > self.monkey_right))):
                #pg.draw.rect(self.screen, (255,0,0), (trunk_left, trunk_top, trunk_right-trunk_left, trunk_bot-trunk_top), 1)
                #pg.draw.rect(self.screen, (255,0,0), (self.monkey_left+15, monkey_top, self.monkey_img.shape[1]-15, monkey_bot-monkey_top), 1)
                if (monkey_top < trunk_top) or (monkey_bot > trunk_bot):
                    tree_hit = True
            
            # Keep score.
            if not tree['s'] and (self.monkey_left+15) > trunk_right:
                tree['s'] = True
                self.score += 1
                pass_tree = True
                # if self.sound:
                #     self.blop_snd.play()

        # Monkey swings down on a vine.
        # if self.vel < 0:
        #     pg.draw.line(self.screen, (92,64,51), (self.screen_width/2+20, self.monkey_loc-25), (self.hook,0), 4)

        # Render the monkey.
        # self.screen.blit(self.monkey_img, (self.monkey_left, monkey_top))

        # Fail on hitting top or bottom.
        if monkey_bot > self.screen_height or monkey_top < 0:
            edge_hit = True

        # Render the score
        # score_text = self.font.render("Score: %d" % (self.score), 1, (230, 40, 40))
        # self.screen.blit(score_text, score_text.get_rect())

        # if self.text is not None:
        #     text = self.font.render(self.text, 1, (230, 40, 40))
        #     textpos = text.get_rect()
        #     self.screen.blit(text, (self.screen_width-textpos[2],0,textpos[2],textpos[3]))

        # # Render the display.
        # pg.display.update()

        # If failed, play sound and exit.  Also, assign rewards.
        if edge_hit:
            # if self.sound:
            #     ch = self.screech_snd.play()
            #     while ch.get_busy():
            #         pg.time.delay(500)
            if self.reward_fn is not None:
                self.reward_fn(self.edge_penalty)
            if self.action_fn is not None:
                self.action_fn(self.get_state())
            return False
        if tree_hit:
            # if self.sound:
            #     ch = self.screech_snd.play()
            #     while ch.get_busy():
            #         pg.time.delay(500)
            if self.reward_fn is not None:
                self.reward_fn(self.tree_penalty)
            if self.action_fn is not None:
                self.action_fn(self.get_state())
            return False

        if self.reward_fn is not None:
            if pass_tree:
                self.reward_fn(self.tree_reward)
            else:
                self.reward_fn(0.0)            
        
        # Wait just a bit.
        # pg.time.delay(self.tick_length)

        # Move things.
        # self.hook -= self.horz_speed
        # self.iter -= self.horz_speed
        # if self.iter < -self.background_img.shape[1]:
        #     self.iter += self.background_img.shape[1]

        return True

if __name__ == '__main__':
    
    # Create the game object.
    game = SwingyMonkey()

    # Loop until you hit something.
    while game.game_loop():
        pass
```
