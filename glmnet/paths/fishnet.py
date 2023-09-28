import logging
import warnings

from typing import Literal
from dataclasses import (dataclass,
                         field)
   
import numpy as np

from sklearn.preprocessing import OneHotEncoder
from sklearn.utils import check_X_y

from .fastnet import FastNetMixin
from .docstrings import (make_docstring,
                         add_dataclass_docstring)

from ._fishnet import fishnet as _dense
from ._fishnet import spfishnet as _sparse

@dataclass
class FishNet(FastNetMixin):

    univariate_beta: bool = True
    type_logistic: Literal['Newton', 'modified_Newton'] = 'Newton'
    _dense = _dense
    _sparse = _sparse

    # private methods

    def _check(self, X, y):

        if np.any(y < 0):
            raise ValueError("negative responses encountered;  not permitted for Poisson family")
        X, y = super()._check(X, y)
        y = np.asarray(y, float)
        return X, y
    
    def _wrapper_args(self,
                      design,
                      y,
                      sample_weight,
                      offset,
                      exclude=[]):
        
        if offset is None:
            offset = 0. * y

        _args = super()._wrapper_args(design,
                                      y,
                                      sample_weight,
                                      offset,
                                      exclude=exclude)

        _args['g'] = np.asfortranarray(offset.reshape((-1,1)))
        return _args
