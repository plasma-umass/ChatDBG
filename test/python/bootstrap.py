from datascience import *
from cs104 import *

def make_marble_bag():
    table = Table().read_table('marble-sample.csv')
    return table.column('color')

def percent_blue(sample):
    return sample 

def main(observed_marbles):
    num_trials = 5 
    stats = bootstrap_statistic(observed_marbles,
                                percent_blue,
                                num_trials)
    assert len(stats) == 5

observed_marbles = make_marble_bag()
main(observed_marbles)
