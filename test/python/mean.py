
import numpy as np
from datascience import *
from ds101 import bootstrap_statistic

def make_marble_bag():
    table = Table().read_table('marble-sample.csv')
    return table.column('color')

observed_marbles = make_marble_bag()

def percent_blue(sample):
    return np.count_nonzero(sample == 'B') / len(sample)

def main():

    num_trials = 5
        
    stats = bootstrap_statistic(observed_marbles,
                                percent_blue,
                                num_trials)

    assert np.isclose(np.mean(stats), 0.7)

main()