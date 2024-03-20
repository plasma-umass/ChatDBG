import numpy as np
from datascience import *

# fake library to hide identities...

def bootstrap_statistic(observed_sample, compute_statistic, num_trials): 
    """
    Creates num_trials resamples of the initial sample.
    Returns an array of the provided statistic for those samples.

    * observed_sample: the initial sample, as an array.
    
    * compute_statistic: a function that takes a sample as 
                         an array and returns the statistic for that
                         sample. 
    
    * num_trials: the number of bootstrap samples to create.

    """

    # Check that observed_sample is an array!
    if not isinstance(observed_sample, np.ndarray):
        raise ValueError('The first parameter to bootstrap_statistic must be a sample represented as an array, not a value of type ' + str(type(observed_sample).__name__))

    statistics = make_array()
    
    for i in np.arange(0, num_trials): 
        #Key: in bootstrapping we must always sample with replacement 
        simulated_resample = np.random.choice(observed_sample, len(observed_sample))
        
        resample_statistic = compute_statistic(simulated_resample)
        statistics = np.append(statistics, resample_statistic)
    
    return statistics
