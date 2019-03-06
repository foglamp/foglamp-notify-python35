/*
 * FogLAMP "Notify Python35" class
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Massimiliano Pinto
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <iostream>

#include "notify_python35.h"

#define SCRIPT_NAME  "notify35"
#define PYTHON_SCRIPT_METHOD_PREFIX "_script_"
#define PYTHON_SCRIPT_FILENAME_EXTENSION ".py"
#define SCRIPT_CONFIG_ITEM_NAME "script"

using namespace std;

/**
 * NotifyPython35 class constructor
 *
 * @param category	The configuration of the idelivery plugin
 */
NotifyPython35::NotifyPython35(ConfigCategory *category)
{
	// Configuration set is protected by a lock
	lock_guard<mutex> guard(m_configMutex);

	m_enabled = false;
	m_pModule = NULL;
	m_pFunc = NULL;
	m_pythonScript = string("");

	m_name = category->getName();

	// Set the enable flag
	if (category->itemExists("enable"))
	{
		m_enabled = category->getValue("enable").compare("true") == 0 ||
			    category->getValue("enable").compare("True") == 0;
	}

	// Check whether we have a Python 3.5 script file to import
	if (category->itemExists(SCRIPT_CONFIG_ITEM_NAME))
	{
		try
		{
			// Get Python script file from "file" attibute of "scipt" item
			m_pythonScript = category->getItemAttribute(SCRIPT_CONFIG_ITEM_NAME,
								    ConfigCategory::FILE_ATTR);
			// Just take file name and remove path
			std::size_t found = m_pythonScript.find_last_of("/");
			if (found != std::string::npos)
			{
				m_pythonScript = m_pythonScript.substr(found + 1);
			}
		}
		catch (ConfigItemAttributeNotFound* e)
		{
			delete e;
		}
		catch (exception* e)
		{
			delete e;
		}
	}

	if (m_pythonScript.empty())
	{
		Logger::getLogger()->warn("Notification plugin '%s', "
					  "called without a Python 3.5 script. "
					  "Check 'script' item in '%s' configuration. "
					  "Notification plugin has been disabled.",
				 	  PLUGIN_NAME,
					  this->getName().c_str());
	}
}

/**
 * NotifyPython35 class destructor
 */
NotifyPython35::~NotifyPython35()
{
}

/**
 * Configure Python35 filter:
 *
 * import the Python script file and call
 * script configuration method with current filter configuration
 *
 * This method must be called while holding the configuration mutex
 *
 * @return	True on success, false on errors.
 */
bool NotifyPython35::configure()
{
	// Import script as module
	// NOTE:
	// Script file name is:
	// lowercase(categoryName) + _script_ + methodName + ".py"

	// 1) Get methodName
	std::size_t found = m_pythonScript.rfind(PYTHON_SCRIPT_METHOD_PREFIX);
	string filterMethod = m_pythonScript.substr(found + strlen(PYTHON_SCRIPT_METHOD_PREFIX));
	// Remove .py from filterMethod
	found = filterMethod.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	filterMethod.replace(found, strlen(PYTHON_SCRIPT_FILENAME_EXTENSION), "");
	// Remove .py from pythonScript
	found = m_pythonScript.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	m_pythonScript.replace(found, strlen(PYTHON_SCRIPT_FILENAME_EXTENSION), "");


	Logger::getLogger()->error("____ NOTIFY PYTHON35 " + m_pythonScript + ",  method = " + filterMethod);

	// 2) Import Python script
	// check first method name (substring of scriptname) has name SCRIPT_NAME
	if (filterMethod.compare(SCRIPT_NAME) != 0)
	{
		Logger::getLogger()->warn("Notification plugin '%s', "
					  "called Python 3.5 script name '%s' does not end with name '%s'. "
					  "Check 'script' item in '%s' configuration. "
					  "Notification plugin has been disabled.",
					  PLUGIN_NAME,
					  m_pythonScript.c_str(),
					  SCRIPT_NAME,
					  this->getName().c_str());

		// Force disable
		this->disableDelivery();

		m_pModule = NULL;
		m_pFunc = NULL;

		return true;
	}

	// Import Python 3.5 module
	m_pModule = PyImport_ImportModule(m_pythonScript.c_str());

	// Check whether the Python module has been imported
	if (!m_pModule)
	{
		// Failure
		if (PyErr_Occurred())
		{
			logErrorMessage();
		}
		Logger::getLogger()->fatal("Notification plugin '%s' (%s), cannot import Python 3.5 script "
					   "'%s' from '%s'",
					   PLUGIN_NAME,
					   this->getName().c_str(),
					   m_pythonScript.c_str(),
					   m_scriptsPath.c_str());

		return false;
	}

	// Fetch filter method in loaded object
	m_pFunc = PyObject_GetAttrString(m_pModule, filterMethod.c_str());
	if (!PyCallable_Check(m_pFunc))
	{
		// Failure
		if (PyErr_Occurred())
		{
			logErrorMessage();
		}

		Logger::getLogger()->fatal("Notification plugin %s (%s) error: cannot "
					   "find Python 3.5 method "
					   "'%s' in loaded module '%s.py'",
					   PLUGIN_NAME,
					   this->getName().c_str(),
					   filterMethod.c_str(),
					   m_pythonScript.c_str());
		Py_CLEAR(m_pModule);
		Py_CLEAR(m_pFunc);

		return false;
	}

	return true;
}

/**
 * Reconfigure the delivery plugin
 *
 * @param newConfig	The new configuration
 */
bool NotifyPython35::reconfigure(const std::string& newConfig)
{
	Logger::getLogger()->error("---Pythin 35 Notify RECONFIGURE called =" + newConfig);

	ConfigCategory category("new", newConfig);

	// Configuration change is protected by a lock
	lock_guard<mutex> guard(m_configMutex);

	// Set the enable flag
        if (category.itemExists("enable"))
        {
                m_enabled = category.getValue("enable").compare("true") == 0 ||
                            category.getValue("enable").compare("True") == 0;
        }

	// Check whether we have a Python 3.5 script file to import
	if (category.itemExists(SCRIPT_CONFIG_ITEM_NAME))
	{
		try
		{
			// Get Python script file from "file" attibute of "scipt" item
			m_pythonScript = category.getItemAttribute(SCRIPT_CONFIG_ITEM_NAME,
								   ConfigCategory::FILE_ATTR);
		        // Just take file name and remove path
			std::size_t found = m_pythonScript.find_last_of("/");
			if (found != std::string::npos)
			{
				m_pythonScript = m_pythonScript.substr(found + 1);
			}
		}
		catch (ConfigItemAttributeNotFound* e)
		{
			delete e;
		}
		catch (exception* e)
		{
			delete e;
		}
	}

	if (m_pythonScript.empty())
	{
		Logger::getLogger()->warn("Notification plugin '%s', "
					  "called without a Python 3.5 script. "
					  "Check 'script' item in '%s' configuration. "
					  "Notification plugin has been disabled.",
					  PLUGIN_NAME,
					  this->getName().c_str());
		// Force disable
		this->disableDelivery();

		return false;
	}

	return this->configure();
}


/**
 * Call Python 3.5 notification method
 *
 * @param notificationName 	The name of this notification
 * @param triggerReason		Why the notification is being sent
 * @param message		The message to send
 */
bool NotifyPython35::notify(const std::string& deliveryName,
			    const string& notificationName,
			    const string& triggerReason,
			    const string& customMessage)
{
	m_configMutex.lock();
        if (!m_enabled)
        {
                // Release lock
                m_configMutex.unlock();
                // Current filter is not active: just return
                return false;
        }

	// Save configuration variables and Python objects
	string name = m_name;
	string scriptName = m_pythonScript;
	PyObject* method = m_pFunc;

	// Release lock
	m_configMutex.unlock();

	Logger::getLogger()->error("_____ PYTHOn35 deliver ...");

	// Call Python method passing an object
	PyObject* pReturn = PyObject_CallFunction(method,
						  "s",
						  customMessage.c_str());

	// Check return status
	if (!pReturn)
	{
		// Errors while getting result object
		Logger::getLogger()->error("Notification plugin '%s' (%s), script '%s', "
					   "filter error, action: %s",
					   PLUGIN_NAME,
					   name.c_str(),
					   scriptName.c_str(),
					   "pass unfiltered data onwards");

		// Errors while getting result object
		logErrorMessage();
	}
	else
	{
		Logger::getLogger()->debug("PyObject_CallFunction() succeeded");

		// Remove pReturn object
		Py_CLEAR(pReturn);
	}

	return pReturn ? true : false;
}

/**
 * Log current Python 3.5 error message
 *
 */
void NotifyPython35::logErrorMessage()
{
        //Get error message
        PyObject *pType, *pValue, *pTraceback;
        PyErr_Fetch(&pType, &pValue, &pTraceback);

        // NOTE from :
        // https://docs.python.org/2/c-api/exceptions.html
        //
        // The value and traceback object may be NULL
        // even when the type object is not.    
        const char* pErrorMessage = pValue ?
                                    PyBytes_AsString(pValue) :
                                    "no error description.";

        Logger::getLogger()->fatal("Notification plugin '%s', Error '%s'",
                                   PLUGIN_NAME,
                                   pErrorMessage ?
                                   pErrorMessage :
                                   "no description");

        // Reset error
        PyErr_Clear();

        // Remove references
        Py_CLEAR(pType);
        Py_CLEAR(pValue);
        Py_CLEAR(pTraceback);
}
