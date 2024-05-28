from datascience import *
from ds101 import *
import pdb

def make_marble_sample():
    table = Table().read_table('marble-sample.csv')
    return table.column('color')

def proportion_blue(sample):
    pdb.set_trace()
    return np.count_nonzero(sample == 'B') / len(sample)

def resampled_stats(observed_marbles, num_trials):
    stats = bootstrap_statistic(observed_marbles,
                                proportion_blue,
                                num_trials)
    assert len(stats) == num_trials
    return stats

def compute_the_stat():
    observed_marbles = make_marble_sample()
    
    return resampled_stats(observed_marbles, 1000)

stats = compute_the_stat()
assert np.isclose(np.mean(stats), 0.5)