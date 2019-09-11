"""
Fledge notification handling script in Python 3.5
"""

__author__ = "Amandeep Singh Arora"
__copyright__ = "Copyright (c) 2018 Dianomic Systems"
__license__ = "Apache 2.0"
__version__ = "${VERSION}"

import logging
from logging.handlers import SysLogHandler


logger = logging.getLogger(__name__)
handler = SysLogHandler(address='/dev/log')

formatter = logging.Formatter(fmt='Fledge[%(process)d] %(levelname)s: %(name)s: %(message)s')
handler.setFormatter(formatter)

logger.setLevel(logging.INFO)
logger.propagate = False
logger.addHandler(handler)


"""
Method for handling notification

Input is the notification message 
"""
def notify35(message):
    global logger
    logger.info("Notification alert: {}".format(message))
    print("Notification alert: {}".format(message))
