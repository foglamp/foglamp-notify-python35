"""
FogLAMP notification handling script in Python 3.5
"""

__author__ = "Amandeep Singh Arora"
__copyright__ = "Copyright (c) 2018 Dianomic Systems"
__license__ = "Apache 2.0"
__version__ = "${VERSION}"

import sys
import json

"""
Method for handling notification

Input is the notification message 
"""
def notify35(message):
    print("Notification alert: " + str(message))

