import numpy as np

red_blue_proportions = np.array([0.3, 0.7])

def make_sample(sample_size, probabilities):
    return np.random.multinomial(sample_size, probabilities)

def make_marble_bag(size):
    return make_sample(red_blue_proportions, size)

make_marble_bag(10)
