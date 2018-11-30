
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <iostream>
#include <utils.h>
#include <plugin_api.h>
#include <logger.h>
#include <config_category.h>

#include <Python.h>

// Relative path to FOGLAMP_DATA
#define PYTHON_FILTERS_PATH "/scripts"
#define PLUGIN_NAME "python35"
#define PYTHON_SCRIPT_METHOD_PREFIX "_script_"
#define PYTHON_SCRIPT_FILENAME_EXTENSION ".py"
#define SCRIPT_CONFIG_ITEM_NAME "script"

#define PRINT_FUNC	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);

using namespace std;

static void logErrorMessage();

typedef struct
{
	PyObject* pModule; // Python 3.5 loaded filter module handle
	PyObject* pFunc; // Python 3.5 callable method handle
	string pythonScript; // Python 3.5 script name
} PLUGIN_INFO;

#define SCRIPT_NAME  "python-notify_script_notify35"

// Filter default configuration
#define DEFAULT_CONFIG "{\"plugin\" : { \"description\" : \"Python 3.5 notification plugin\", " \
                       		"\"type\" : \"string\", " \
				"\"default\" : \"" PLUGIN_NAME "\", " \
				"\"value\" : \"" PLUGIN_NAME "\" }, " \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the Python 3.5 notification plugin.\", " \
				"\"type\": \"boolean\", " \
				"\"default\": \"false\", " \
				"\"value\": \"false\" }, " \
			"\"config\" : {\"description\" : \"Python 3.5 filter configuration.\", " \
				"\"type\" : \"JSON\", " \
				"\"value\" : {}, " \
				"\"default\" : {} }, " \
			"\"script\" : {\"description\" : \"Python 3.5 module to load.\", " \
				"\"type\": \"string\", " \
			    "\"default\": \"" SCRIPT_NAME "\", " \
			    "\"file\": \"" SCRIPT_NAME "\", " \
				"\"value\": \"" SCRIPT_NAME "\"} }"

#define GET_PLUGIN_INFO_ELEM(elem) \
{ \
		PyObject* item = PyDict_GetItemString(element, #elem);\
		char * elem = new char [string(PyBytes_AsString(item)).length()+1];\
		std::strcpy (elem, string(PyBytes_AsString(item)).c_str());\
		info->elem = elem;\
}

PyObject *C2Py_ConfigCategory(ConfigCategory &config)
{
	Logger::getLogger()->info("config category count=%d, json=%s", config.getCount(), config.toJSON().c_str()); 
	for (auto const &item : config)
	{
		string json = item->toJSON();
		PRINT_FUNC;
		Logger::getLogger()->info("config item json=%s", json.c_str());
		Logger::getLogger()->info("config item name=%s, description=%s, type=%s", item->getName().c_str(), item->getDescription().c_str(), item->getType().c_str());
	}
}

/**
 * Get PLUGIN_INFORMATION structure filled from Python object
 *
 * @param pyRetVal	Python 3.5 Object (dict)
 * @return		Pointer to a new PLUGIN_INFORMATION structure
 *				or NULL in case of errors
 */
PLUGIN_INFORMATION *Py2C_PluginInfo(PyObject* pyRetVal)
{
#if 0
	//PyObject* pyRetVal = Py_BuildValue("{s:s,s:s}", "name", "pythonPlugin", "version", "1.0.0");	  //{'abc': 123, 'def': 456}
	PyObject *pyRetVal = PyDict_New(); // new reference

	// add a few named values
	//PyDict_SetItemString(pyRetVal, "name", Py_BuildValue("s", "pythonPlugin"));
	//PyDict_SetItemString(pyRetVal, "version", Py_BuildValue("s", "1.0.0"));

	PyObject* nameKey = PyBytes_FromString("pythonPlugin");
	PyDict_SetItemString(pyRetVal, "name", nameKey);

	PyObject* verKey = PyBytes_FromString("1.0.0");
	PyDict_SetItemString(pyRetVal, "version", verKey);
	
#endif
	
	// Create returnable PLUGIN_INFORMATION structure
	PLUGIN_INFORMATION *info = new PLUGIN_INFORMATION;

	//Logger::getLogger()->info("PyDict_Size(pyRetVal)=%d, PyDict_Check(pyRetVal)=%s", PyDict_Size(pyRetVal), PyDict_Check(pyRetVal)?"true":"false"); 

#if 0
	PyObject *keys = PyDict_Keys(pyRetVal);
	for (int i = 0; i < PyList_Size(keys); i++)
	{
		// Get list item: borrowed reference.
		PyObject* element = PyList_GetItem(keys, i);
		const char* type = Py_TYPE(element)->tp_name;
		Logger::getLogger()->info("0.5 i=%d, PyUnicode_Check(element)=%s, type=%s", i, PyUnicode_Check(element)?"true":"false", type);
		if (!element)
		{
			// Failure
			if (PyErr_Occurred())
			{
				logErrorMessage();
			}
			Logger::getLogger()->info("1. PyDict: keys[%d] is not valid", i);
		}

		Logger::getLogger()->info("2. PyDict: keys[%d]=%s", i, PyUnicode_AsUTF8(element));
	}
#endif

#if 0
	PyObject *pKeys = PyDict_Keys(pyRetVal); // new reference
	for(int i = 0; i < PyList_Size(pKeys); ++i)
	{
	
		PyObject *pKey =
				PyList_GetItem(pKeys, i); // borrowed reference
	
		PyObject *pValue =
				PyDict_GetItem(pyRetVal, pKey); // borrowed reference

		Logger::getLogger()->info("2.1 PyBytes_Check(pKey)=%s, PyBytes_Check(pValue)=%s", PyBytes_Check(pKey)?"true":"false", PyBytes_Check(pValue)?"true":"false");
	}
	
	Py_DECREF(pKeys);
#endif

#if 1
	PyObject *dKey, *dValue;
	Py_ssize_t dPos = 0;
	
	// dKey and dValue are borrowed references
	while (PyDict_Next(pyRetVal, &dPos, &dKey, &dValue))
	{
		 /*if (!PyBytes_Check(dKey) || !PyBytes_Check(dValue))
		  {
			Logger::getLogger()->info("3. PyDict: dKey & dValue are not of required type");
			continue;
		  }*/
		char* ckey = PyUnicode_AsUTF8(dKey);
		char* cval;
		char *emptyStr = new char[1];
		emptyStr[0] = '\0';
		if(strcmp(ckey, "config"))
			cval = PyUnicode_AsUTF8(dValue);
		else
			cval = emptyStr;
		//Logger::getLogger()->info("4. PyDict: dKey=%s, dValue=%s", ckey, cval);

		char *valStr = new char [string(cval).length()+1];
		std::strcpy (valStr, cval);
		
		if(!strcmp(ckey, "name"))
		{
			info->name = valStr;
		}
		else if(!strcmp(ckey, "version"))
		{
			info->version = valStr;
		}
		else if(!strcmp(ckey, "mode"))
		{
			info->options = 0;
			if (!strcmp(valStr, "async"))
			info->options |= SP_ASYNC;
			free(valStr);
		}
		else if(!strcmp(ckey, "type"))
		{
			info->type = valStr;
		}
		else if(!strcmp(ckey, "interface"))
		{
			info->interface = valStr;
		}
		else if(!strcmp(ckey, "config"))
		{
			free (valStr);
			char *valStr1 = new char[3];
			valStr1[0]='{';
			valStr1[1]='}';
			valStr1[2]='\0';
			info->config = valStr1;
			// TODO : write real code : probably convert python dict to JSON string
		}
			
#if 0
		if (PyUnicode_AsUTF8(dKey) == "name")
		{
			PyObject* item = dValue;
			Logger::getLogger()->info("5. PyDict: name item=%p", item);
			Logger::getLogger()->info("6. PyDict: name=%s", PyUnicode_AsUTF8(item));
			if (item && PyBytes_Check(item))
			{
				// Set name
				Logger::getLogger()->info("7. PyDict: name=%s", PyUnicode_AsUTF8(item));
				char *name = new char [string(PyUnicode_AsUTF8(item)).length()+1];
				std::strcpy (name, string(PyUnicode_AsUTF8(item)).c_str());
				info->name = name;
			}
		}
#endif
	}
#endif

#if 0
	// Iterate filtered data in the list
	for (int i = 0; i < PyList_Size(pyRetVal); i++)
	{
		// Get list item: borrowed reference.
		PyObject* element = PyList_GetItem(pyRetVal, i);
		if (!element)
		{
			// Failure
			if (PyErr_Occurred())
			{
				logErrorMessage();
			}
			delete info;

			return NULL;
		}

#if 0
		GET_PLUGIN_INFO_ELEM(name)
		GET_PLUGIN_INFO_ELEM(version)
		GET_PLUGIN_INFO_ELEM(type)
		GET_PLUGIN_INFO_ELEM(interface)
		GET_PLUGIN_INFO_ELEM(config)
#else
		PyObject* item;
		// Get 'name' value: borrowed reference
		item = PyDict_GetItemString(element, "name");
		char *name = new char [string(PyBytes_AsString(item)).length()+1];
		std::strcpy (name, string(PyBytes_AsString(item)).c_str());
		info->name = name;

		// Get 'version' value: borrowed reference
		item = PyDict_GetItemString(element, "version");
		char *version = new char [string(PyBytes_AsString(item)).length()+1];
		std::strcpy (version, string(PyBytes_AsString(item)).c_str());
		info->version = version;
#endif
		
		// Get 'name' value: borrowed reference
		item = PyDict_GetItemString(element, "options");
		if (!item)
		{
			// Failure
			if (PyErr_Occurred())
			{
				logErrorMessage();
			}
			delete info;
			return NULL;
		}
		
		info->options = PyLong_AsUnsignedLong(item);
#if 0
		// Get 'type' value: borrowed reference
		item = PyDict_GetItemString(element, "type");
		char *type = new char [string(PyBytes_AsString(item)).length()+1];
		std::strcpy (type, string(PyBytes_AsString(item)).c_str());
		info->type = type;

		// Get 'name' value: borrowed reference
		item = PyDict_GetItemString(element, "interface");
		char *type = new char [string(PyBytes_AsString(item)).length()+1];
		std::strcpy (type, string(PyBytes_AsString(item)).c_str());
		info->type = type;

		// Get 'name' value: borrowed reference
		item = PyDict_GetItemString(element, "config");
		char *type = new char [string(PyBytes_AsString(item)).length()+1];
		std::strcpy (type, string(PyBytes_AsString(item)).c_str());
		info->type = type;
#endif
	}
#endif

	return info;
}



int main(int argc, char *argv[])
{
	Logger::getLogger()->info("%s:%s - start", __FILE__, __FUNCTION__);
	
	PLUGIN_INFO *info = new PLUGIN_INFO;
	info->pythonScript = string("");

	ConfigCategory defCfg(string("python-notify"), string(DEFAULT_CONFIG));

	Logger::getLogger()->info("defCfg=%s", defCfg.toJSON().c_str());

	// Check whether we have a Python 3.5 script file to import
	if (defCfg.itemExists(SCRIPT_CONFIG_ITEM_NAME))
	{
		try
		{
			// Get Python script file from "file" attibute of "scipt" item
			info->pythonScript = defCfg.getItemAttribute(SCRIPT_CONFIG_ITEM_NAME,
									    ConfigCategory::FILE_ATTR);
		        // Just take file name and remove path
			std::size_t found = info->pythonScript.find_last_of("/");
			info->pythonScript = info->pythonScript.substr(found + 1);
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

	Logger::getLogger()->info("%s:%d - info->pythonScript=%s", __FUNCTION__, __LINE__, info->pythonScript.c_str());

	if (info->pythonScript.empty())
	{
		// Do nothing
		Logger::getLogger()->warn("Notification plugin '%s', "
					  "called without a Python 3.5 script. "
					  "Check 'script' item in '%s' configuration. "
					  "Notification plugin has been disabled.",
					  PLUGIN_NAME,
					  defCfg.getName().c_str());

		// Return filter handle
		return -1;
	}
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
		
	// Embedded Python 3.5 program name
	wchar_t *programName = Py_DecodeLocale(defCfg.getName().c_str(), NULL);
    Py_SetProgramName(programName);
	PyMem_RawFree(programName);
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	// Embedded Python 3.5 initialisation
    Py_Initialize();
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);

	// Get FogLAMP Data dir
	string filtersPath = getDataDir();
	// Add filters dir
	filtersPath += PYTHON_FILTERS_PATH;
	Logger::getLogger()->info("%s:%d: filtersPath=%s, info->pythonScript=%s", __FUNCTION__, __LINE__, filtersPath.c_str(), info->pythonScript.c_str());

	// Set Python path for embedded Python 3.5
	// Get current sys.path. borrowed reference
	PyObject* sysPath = PySys_GetObject((char *)string("path").c_str());
	//PyList_Append(sysPath, PyUnicode_FromString((char *) filtersPath.c_str()));
	//PyList_Append(sysPath, PyUnicode_FromString((char *) "."));
	PyList_Append(sysPath, PyUnicode_FromString((char *) "/home/foglamp/dev/FogLAMP/foglamp-notify-python35/sinusoid"));
	PyList_Append(sysPath, PyUnicode_FromString((char *) "/home/foglamp/dev/FogLAMP/python"));
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);

	// Import script as module
	// NOTE:
	// Script file name is:
	// lowercase(categoryName) + _script_ + methodName + ".py"
	// python-notify_script_notify35.py

	// 1) Get methodName
#if 0
	std::size_t found = info->pythonScript.rfind(PYTHON_SCRIPT_METHOD_PREFIX);
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	string filterMethod = info->pythonScript.substr(found + strlen(PYTHON_SCRIPT_METHOD_PREFIX));
	Logger::getLogger()->info("%s:%d - filterMethod=%s", __FUNCTION__, __LINE__, filterMethod.c_str());
	// Remove .py from filterMethod
	found = filterMethod.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	filterMethod.replace(found,
			     strlen(PYTHON_SCRIPT_FILENAME_EXTENSION),
			     "");
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	// Remove .py from pythonScript
	found = info->pythonScript.rfind(PYTHON_SCRIPT_FILENAME_EXTENSION);
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	info->pythonScript.replace(found,
			     strlen(PYTHON_SCRIPT_FILENAME_EXTENSION),
			     "");
#else
	info->pythonScript = "sinusoid";
	string filterMethod = "plugin_info";
#endif
	Logger::getLogger()->info("%s:%d - info->pythonScript=%s, filterMethod=%s", __FUNCTION__, __LINE__, info->pythonScript.c_str(), filterMethod.c_str());

	// Import Python script
	info->pModule = PyImport_ImportModule(info->pythonScript.c_str());
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);

	// Check whether the Python module has been imported
	if (!info->pModule)
	{
		// Failure
		if (PyErr_Occurred())
		{
			logErrorMessage();
		}
		Logger::getLogger()->fatal("Notification plugin '%s' (%s), cannot import Python 3.5 script "
					   "'%s' from '%s'",
					   PLUGIN_NAME,
					   defCfg.getName().c_str(),
					   info->pythonScript.c_str(),
					   filtersPath.c_str());

		// This will abort the filter pipeline set up
		return -1;
	}

	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
	// Fetch filter method in loaded object
	info->pFunc = PyObject_GetAttrString(info->pModule, filterMethod.c_str());
	Logger::getLogger()->info("%s:%d: info->pFunc=%p", __FUNCTION__, __LINE__, info->pFunc);

	if (!info->pFunc || !PyCallable_Check(info->pFunc))
	{
		// Failure
		if (PyErr_Occurred())
		{
			logErrorMessage();
		}

		Logger::getLogger()->fatal("Notification plugin %s (%s) error: cannot find Python 3.5 method "
					   "'%s' in loaded module '%s'",
					   PLUGIN_NAME,
					   defCfg.getName().c_str(),
					   filterMethod.c_str(),
					   info->pythonScript.c_str());
		Py_CLEAR(info->pModule);
		Py_CLEAR(info->pFunc);

		// This will abort the filter pipeline set up
		return -1;
	}

	string message("Mayday! Mayday! Notification alert");

	// Call Python method passing an object
	PyObject* pReturn = PyObject_CallObject(info->pFunc, NULL);

	// - 3 - Handle filter returned data
	if (!pReturn)
	{
		// Errors while getting result object
		Logger::getLogger()->error("Notification plugin '%s' (%s), script '%s', "
					   "filter error, action: %s",
					   PLUGIN_NAME,
					   defCfg.getName().c_str(),
					   info->pythonScript.c_str(),
					   "pass unfiltered data onwards");

		// Errors while getting result object
		logErrorMessage();
	}
	else
	{
		Logger::getLogger()->info("PyObject_CallFunction() succeeded");
		
		PLUGIN_INFORMATION *info = Py2C_PluginInfo(pReturn);
		if(info)
		Logger::getLogger()->info("plugin_handle: plugin_info(): info = { name=%s, version=%s, options=%d, type=%s, interface=%s, config=%s }", 
					info->name, info->version, info->options, info->type, info->interface, info->config);
		
		// Remove pReturn object
		Py_CLEAR(pReturn);
	}

	ConfigCategory config("sinusoidCfg", DEFAULT_CONFIG);
	PyObject *pyCfg = C2Py_ConfigCategory(config);
	
}

static void logErrorMessage()
{
	Logger::getLogger()->info("%s:%d", __FUNCTION__, __LINE__);
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

	Logger::getLogger()->fatal("Filter '%s', script "
				   "'%s': Error '%s'",
				   PLUGIN_NAME,
				   "notify35",
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

