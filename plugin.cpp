/**
 * FogLAMP SimpleExpression notification rule plugin
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora, Massimiliano Pinto
 */

#include <plugin_api.h>
#include <logger.h>
#include <plugin_exception.h>
#include <iosfwd>
#include <config_category.h>
#include <rapidjson/writer.h>
#include <builtin_rule.h>
#include "version.h"
#include "simple_expression.h"

#define RULE_NAME "SimpleExpression"
#define RULE_DESCRIPTION  "Generate a notification based on the evaluation of a user provided expression"

/**
 * Plugin configuration
 *
 * Example:
    {
		"asset": {
			"description": "The asset name for which notifications will be generated.",
			"name": "modbus"
		},
		"expression": {
			"description": "The expression to evaluate",
			"name": "Expression",
			"type": "string",
			"value": "humidity > 50"
		}
	}
 
 * Expression is composed of datapoint values within given asset name.
 * And if the value of boolean expression toggles, then the notification is sent.
 *
 * NOTE:
 * Datapoint names and values are dynamically added when "plugin_eval" is called
 */


const char * defaultConfiguration = QUOTE(
{
	"plugin" : {
		"description" : RULE_DESCRIPTION,
		"type" : "string",
		"default" :  RULE_NAME,
		"readonly" : "true"
	},
	"description" : {
		"description" : "Generate a notification using an expression evaluation.",
		"type" : "string",
		"default" : "Generate a notification using an expression evaluation.",
		"displayName" : "Rule",
		"readonly" : "true"
	},
	"asset" : {
		"description" : "The asset name for which notifications will be generated.",
		"type" : "string",
		"default" : "",
		"displayName" : "Asset name",
		"order" : "1"
	},
	"expression" : {
		"description" : "Expression to apply.",
		"name" : "Expression",
		"type" : "string",
		"default": "",
		"displayName" : "Expression to apply",
		"order" : "2"
	}
});

using namespace std;

/**
 * The C plugin interface
 */
extern "C" {
/**
 * The C API rule information structure
 */
static PLUGIN_INFORMATION ruleInfo = {
	RULE_NAME,			// Name
	VERSION,			// Version
	0,				// Flags
	PLUGIN_TYPE_NOTIFICATION_RULE,	// Type
	"1.0.0",			// Interface version
	defaultConfiguration		// Configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &ruleInfo;
}

/**
 * Initialise rule objects based in configuration
 *
 * @param    config	The rule configuration category data.
 * @return		The rule handle.
 */
PLUGIN_HANDLE plugin_init(const ConfigCategory& config)
{
	SimpleExpression* handle = new SimpleExpression();
	bool rv = handle->configure(config);

	if(rv == false)
	{
		delete handle;
		Logger::getLogger()->info("plugin_init failed");
		handle = NULL;
	}

	return (PLUGIN_HANDLE)handle;
}

/**
 * Free rule resources
 */
void plugin_shutdown(PLUGIN_HANDLE handle)
{
	SimpleExpression* rule = (SimpleExpression *)handle;
	// Delete plugin handle
	delete rule;
}

/**
 * Return triggers JSON document
 *
 * @return	JSON string
 */
string plugin_triggers(PLUGIN_HANDLE handle)
{
	string ret;
	SimpleExpression* rule = (SimpleExpression *)handle;

	// Configuration fetch is protected by a lock
	rule->lockConfig();

	if (!rule->hasTriggers())
	{
		ret = "{\"triggers\" : []}";
		rule->unlockConfig();
		return ret;
	}

	ret = "{\"triggers\" : [ ";
	std::map<std::string, RuleTrigger *> triggers = rule->getTriggers();
	for (auto it = triggers.begin();
		  it != triggers.end();
		  ++it)
	{
		ret += "{ \"asset\"  : \"" + (*it).first + "\"";
		ret += " }";		
		if (std::next(it, 1) != triggers.end())
		{
			ret += ", ";
		}
	}

	ret += " ] }";

	// Release lock
	rule->unlockConfig();

	Logger::getLogger()->debug("plugin_triggers(): ret=%s", ret.c_str());

	return ret;
}

/**
 * Evaluate notification data received
 *
 *  Note: all assets must trigger in order to return TRUE
 *
 * @param    assetValues	JSON string document
 *				with notification data.
 * @return			True if the rule was triggered,
 *				false otherwise.
 */
bool plugin_eval(PLUGIN_HANDLE handle,
		 const string& assetValues)
{
	Logger::getLogger()->debug("plugin_eval(): assetValues=%s", assetValues.c_str());
	Document doc;
	doc.Parse(assetValues.c_str());
	if (doc.HasParseError())
	{
		return false;
	}

	bool eval = false;
	SimpleExpression* rule = (SimpleExpression *)handle;

	map<std::string, RuleTrigger *>& triggers = rule->getTriggers();

	// Iterate throgh all configured assets
	// If we have multiple asset the evaluation result is
	// TRUE only if all assets checks returned true
	for (auto & t : triggers)
	{
		string assetName = t.first;
		if (!doc.HasMember(assetName.c_str()))
		{
			eval = false;
		}
		else
		{
			// Get all datapoints for assetName
			const Value& assetValue = doc[assetName.c_str()];

			// Set evaluation
			eval = rule->evalAsset(assetValue);
		}
	}

	// Set final state: true is all calls to evalAsset() returned true
	rule->setState(eval);

	return eval;
}

/**
 * Return rule trigger reason: trigger or clear the notification. 
 *
 * @return	 A JSON string
 */
string plugin_reason(PLUGIN_HANDLE handle)
{
	SimpleExpression* rule = (SimpleExpression *)handle;
	// Get state, assets and timestamp
	BuiltinRule::TriggerInfo info;
	rule->getFullState(info);

	string ret = "{ \"reason\": \"";
	ret += info.getState() == BuiltinRule::StateTriggered ? "triggered" : "cleared";
	ret += "\"";
	ret += ", \"asset\": " + info.getAssets() + ", \"timestamp\": \"" + info.getUTCDateTime() + "\"";
	ret += " }";

	Logger::getLogger()->debug("plugin_reason(): ret=%s", ret.c_str());

	return ret;
}

/**
 * Call the reconfigure method in the plugin
 *
 * Not implemented yet
 *
 * @param    newConfig		The new configuration for the plugin
 */
void plugin_reconfigure(PLUGIN_HANDLE handle,
			const string& newConfig)
{
	SimpleExpression* rule = (SimpleExpression *)handle;
	ConfigCategory  config("newCfg", newConfig);
	bool rv = rule->configure(config);

	if(rv == false)
		Logger::getLogger()->info("plugin_reconfigure failed");
}

// End of extern "C"
};

/**
 * Evaluate datapoints values for the given asset name
 *
 * @param    assetValue		JSON object with datapoints
 *
 * @return		True if evalution succeded,
 *				false otherwise.
 */
bool SimpleExpression::evalAsset(const Value& assetValue)
{
	bool foundDatapoints = false;
	bool assetEval = false;
	
	for (auto &m : assetValue.GetObject())
	{
		foundDatapoints = true;
		double value;
		if (m.value.IsDouble())
		{
			value = (double) m.value.GetDouble();
		}
		else if (m.value.IsNumber())
		{
			value = (double) m.value.GetInt();
		}
		else
		{
			value = 0.0;
		}

		if (m.value.IsDouble() ||
		    m.value.IsNumber())
		{
			// Add variable
			this->getEvaluator()->addVariable(m.name.GetString(), value);
		}
	}

	if (!foundDatapoints)
	{
		Logger::getLogger()->info("Couldn't find any valid datapoint in plugin_eval input data");
		return false;
	}

	typedef exprtk::parser_error::type error_t;

	// Get expression from config
	string expression = this->getTrigger();

	// Update SymbolTable of Evaluator
	this->getEvaluator()->registerSymbolTable();

	// Parse and comoile expression with variables
	if (!this->getEvaluator()->parserCompile(expression))
	{
		Logger::getLogger()->error("Failed to compile expression: Error: %s\tExpression: %s",
					   this->getEvaluator()->getError().c_str(),
					   expression.c_str());

		return false;
	}

	// Evaluate the expression
	double evaluation = this->getEvaluator()->evaluate();

	Logger::getLogger()->debug("SimpleExpression::Evaluator::evaluate(): m_expression.value()=%lf",
				   evaluation);

	// Checks
	if (std::isnan(evaluation) || !isfinite(evaluation))
	{
		Logger::getLogger()->error("SimpleExpression::evalAsset(): unable to evaluate expression");
	}

	// Set result
	assetEval = (evaluation ==  1.0);

	Logger::getLogger()->debug("m_triggerExpression->evaluate() returned assetEval=%s",
				   assetEval ? "true" : "false");

	// Return evaluation for current asset
	return assetEval;
}

/**
 * SimpleExpression rule constructor
 *
 * Call parent class BuiltinRule constructor
 * passing a plugin handle
 */
SimpleExpression::SimpleExpression() : BuiltinRule()
{
	m_triggerExpression = new Evaluator();
}

/**
 * SimpleExpression destructor
 */
SimpleExpression::~SimpleExpression()
{
	delete m_triggerExpression;
}

/**
 * Configure the rule plugin
 *
 * @param    config	The configuration object to process
 */
bool SimpleExpression::configure(const ConfigCategory& config)
{
	string assetName =  config.getValue("asset");
	string expression =  config.getValue("expression");

	if (assetName.empty() ||
	    expression.empty())
	{
		Logger::getLogger()->warn("Empty values for 'asset' or 'expression'");

		// Return true, so it can be configured later
		return true;
	}

	this->lockConfig();

	if (m_triggerExpression)
	{
		delete m_triggerExpression;
		m_triggerExpression = new Evaluator();
	}
	this->setTrigger(expression);

	if (this->hasTriggers())
	{       
		this->removeTriggers();
	}
	this->addTrigger(assetName, NULL);
		
	// Release lock
	this->unlockConfig();

	return true;
}

/**
 * Constructor for the evaluator class. This holds the expressions and
 * variable bindings used to execute the triggers.
 */
SimpleExpression::Evaluator::Evaluator() : m_varCount(0)
{
	bool rv = m_symbolTable.add_constants();
	if (rv == false)
	{
		Logger::getLogger()->error("m_symbolTable.add_constants() failed");
		throw new exception();
	}
	m_expression.register_symbol_table(m_symbolTable);
}

/**
 * Add a variable and its value to Evaluator symbolTable
 * If variable is already present just update the value
 *
 * We can add up to MAX_EXPRESSION_VARIABLES variables
 */
void SimpleExpression::Evaluator::addVariable(const std::string& dapointName,
					      double value)
{
	if (!m_varCount)
	{
		m_variableNames[0] = dapointName;
		m_variables[0] = value;
		m_symbolTable.add_variable(m_variableNames[0],
					   m_variables[0]);
		m_varCount++;
	}
	else
	{
		bool found = false;
		for (int i = 0; i < m_varCount; i++)
		{
			if (m_variableNames[i].compare(dapointName) == 0)
			{
				found = true;
				m_variables[i] = value;
				break;
			}
		}

		if (!found)
		{
			if (m_varCount < MAX_EXPRESSION_VARIABLES)
			{
				m_variableNames[m_varCount] = dapointName;
				m_variables[m_varCount] = value;
				m_symbolTable.add_variable(m_variableNames[m_varCount],
							   m_variables[m_varCount]);
				m_varCount++;
			}
			else
			{
				Logger::getLogger()->warn("Already set %d variables, can not add the new one '%s'",
							  MAX_EXPRESSION_VARIABLES,
							  dapointName.c_str());
			}
		}
	}
}
