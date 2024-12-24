from datascience import *
from ds101 import *

def make_marble_sample():
    table = Table().read_table('marble-sample.csv')
    return table.column('color')

def proportion_blue(sample):
    return sample

def resampled_stats(observed_marbles, num_trials):
    stats = bootstrap_statistic(observed_marbles,
                                proportion_blue,
                                num_trials)
    assert len(stats) == num_trials
    return stats

observed_marbles = make_marble_sample()
stats = resampled_stats(observed_marbles, 5)

assert np.isclose(np.mean(stats), 0.7)