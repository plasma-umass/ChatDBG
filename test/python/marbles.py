from datascience import *
from ds101 import *

def make_marble_bag():
    table = Table().read_table('marble-sample.csv')
    return table.column('color')

def ratio(x,y):
    return x / y

def ratio_blue_to_red(sample):
    blues = np.count_nonzero(sample == 'B') 
    reds = np.count_nonzero(sample == 'r')
    return ratio(blues, reds)

marbles = make_marble_bag()
if 'R' in marbles:
    print(ratio_blue_to_red(marbles))
