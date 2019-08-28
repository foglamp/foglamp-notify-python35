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
 * @param category	The configuration of the delivery plugin
 */
NotifyPython35::NotifyPython35(ConfigCategory *category)
{
	// Configuration set is protected by a lock
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
 * Configure Python35 plugin:
 *
 * import the Python script file and call
 * script configuration method with current plugin configuration
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

	string filterMethod;
	std::size_t found;

	// 1) Get methodName
	found = m_pythonScript.rfind(PYTHON_SCRIPT_METHOD_PREFIX);
	if (found != std::string::npos)
	{
		filterMethod = m_pythonScript.substr(found + strlen(PYTHON_SCRIPT_METHOD_PREFIX));
	}
	// Remove .py from filterMethod
	found = filterMethod.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	if (found != std::string::npos)
	{
		filterMethod.replace(found, strlen(PYTHON_SCRIPT_FILENAME_EXTENSION), "");
	}
	// Remove .py from pythonScript
	found = m_pythonScript.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	if (found != std::string::npos)
	{
		m_pythonScript.replace(found, strlen(PYTHON_SCRIPT_FILENAME_EXTENSION), "");
	}

	Logger::getLogger()->debug("%s delivery plugin: script='%s', method='%s'",
				   PLUGIN_NAME,
				   m_pythonScript.c_str(),
				   filterMethod.c_str());

	// 2) Import Python script
	// check first method name is empty:
	// disable delivery, cleanup and return true
	// This allows reconfiguration
        if (filterMethod.empty())
	{
		// Force disable
		this->disableDelivery();

		m_pModule = NULL;
		m_pFunc = NULL;

		return true;
	}

	// 2) Import Python script if module object is not set
	if (!m_pModule)
	{
		m_pModule = PyImport_ImportModule(m_pythonScript.c_str());
	}

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
	Logger::getLogger()->debug("%s notification 'plugin_reconfigure' called = %s",
				   PLUGIN_NAME,
				   newConfig.c_str());

	ConfigCategory category("new", newConfig);

	// Configuration change is protected by a lock
	lock_guard<mutex> guard(m_configMutex);

	PyGILState_STATE state = PyGILState_Ensure(); // acquire GIL

	// Reimport module
	PyObject* newModule = PyImport_ReloadModule(m_pModule);

	// Cleanup Loaded module
	Py_CLEAR(m_pModule);
	m_pModule = NULL;
	Py_CLEAR(m_pFunc);
	m_pFunc = NULL;
	m_pythonScript.clear();

	// Set reloaded module
	m_pModule = newModule;

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

		PyGILState_Release(state);

		return false;
	}

	bool ret = this->configure();

	PyGILState_Release(state);

	return ret;
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
	lock_guard<mutex> guard(m_configMutex);
	bool ret = false;

        if (!m_enabled && !Py_IsInitialized())
        {
                // Current plugin is not active: just return
                return false;
        }

	PyGILState_STATE state = PyGILState_Ensure();

	// Save configuration variables and Python objects
	string name = m_name;
	string scriptName = m_pythonScript;
	PyObject* method = m_pFunc;

	// Call Python method passing an object
	PyObject* pReturn = PyObject_CallFunction(method,
						  "s",
						  customMessage.c_str());

	// Check return status
	if (!pReturn)
	{
		// Errors while getting result object
		Logger::getLogger()->error("Notification plugin '%s' (%s), error in script '%s', "
					   "error",
					   PLUGIN_NAME,
					   name.c_str(),
					   scriptName.c_str());

		// Errors while getting result object
		logErrorMessage();
	}
	else
	{
		ret = true;
		Logger::getLogger()->debug("PyObject_CallFunction() succeeded");

		// Remove pReturn object
		Py_CLEAR(pReturn);
	}

	Logger::getLogger()->debug("Notification '%s' 'plugin_delivery' "
				   "called, return = %d",
				   this->getName().c_str(),
				   ret);

	PyGILState_Release(state);

	return ret;
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
        PyErr_NormalizeException(&pType, &pValue, &pTraceback);

        PyObject* str_exc_value = PyObject_Repr(pValue);
        PyObject* pyExcValueStr = PyUnicode_AsEncodedString(str_exc_value, "utf-8", "Error ~");

        // NOTE from :
        // https://docs.python.org/2/c-api/exceptions.html
        //
        // The value and traceback object may be NULL
        // even when the type object is not.    
        const char* pErrorMessage = pValue ?
                                    PyBytes_AsString(pyExcValueStr) :
                                    "no error description.";

        Logger::getLogger()->fatal("Notification plugin '%s', Error '%s'",
                                   PLUGIN_NAME,
                                   pErrorMessage);

        // Reset error
        PyErr_Clear();

        // Remove references
        Py_CLEAR(pType);
        Py_CLEAR(pValue);
        Py_CLEAR(pTraceback);
        Py_CLEAR(str_exc_value);
        Py_CLEAR(pyExcValueStr);
}
