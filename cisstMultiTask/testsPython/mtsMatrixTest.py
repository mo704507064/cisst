# -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ex: set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:

#
#

# Author: Anton Deguet
# Date: 2010-01-20
#
# (C) Copyright 2010 Johns Hopkins University (JHU), All Rights
# Reserved.

# --- begin cisst license - do not edit ---
# 
# This software is provided "as is" under an open source license, with
# no warranty.  The complete license can be found in license.txt and
# http://www.cisst.org/cisst/license.txt.
# 
# --- end cisst license ---

import unittest
import numpy

import cisstCommonPython
import cisstVectorPython
import cisstMultiTaskPython

class MatrixTest(unittest.TestCase):
    def setUp(self):
        """Call before every test case."""

    def tearDown(self):
        """Call after every test case."""
        
    def TestType(self):
        """Test mtsIntMat"""
        variable = cisstMultiTaskPython.mtsIntMat(3, 4)
        # check type
        self.failUnless(isinstance(variable, cisstMultiTaskPython.mtsIntMat))
        self.failUnless(isinstance(variable, cisstMultiTaskPython.mtsGenericObject))
        self.failUnless(isinstance(variable, cisstCommonPython.cmnGenericObject))

    def TestDefaultConstructor(self):
        """Test default constructor"""
        variable = cisstMultiTaskPython.mtsDoubleMat()
        self.failUnless(isinstance(variable, cisstMultiTaskPython.mtsDoubleMat))
        data = variable.Data()
        # this is an array
        self.failUnless(isinstance(data, numpy.ndarray))
        # dimension is 2
        self.failUnless(numpy.ndim(data) == 2)
        # size is 0 by default
        self.failUnless(data.size == 0)

    def TestSizeConstructor(self):
        """Test constructor with size"""
        variable = cisstMultiTaskPython.mtsDoubleMat(10, 5)
        self.failUnless(isinstance(variable, cisstMultiTaskPython.mtsDoubleMat))
        data = variable.Data()
        # this is an array
        self.failUnless(isinstance(data, numpy.ndarray))
        # dimension is 2
        self.failUnless(numpy.ndim(data) == 2)
        # size is 10 * 5 based on constructor parameter
        self.failUnless(data.size == 10 * 5)
        self.failUnless(data.shape == (10, 5))
        # make sure content is zeros
        zeroMatrix = numpy.zeros((10, 5), numpy.float)
        allEqual = (zeroMatrix == data).all()
        self.failUnless(allEqual)
        
    def TestModify(self):
        """Test data modification"""
        variable = cisstMultiTaskPython.mtsDoubleMat(3, 7)
        # data is a reference so modifying it should
        data = variable.Data()
        data.fill(5.0)
        data2 = variable.Data()
        # these are two different objects (Python)
        self.failUnless(data is not data2)
        # but they should be equal
        allEqual = (data2 == data).all()
        self.failUnless(allEqual)
        # always!
        data2[2, 2] = 10.0
        allEqual = (data2 == data).all()
        self.failUnless(allEqual)
        
