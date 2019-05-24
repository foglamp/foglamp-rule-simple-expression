/**
 * FogLAMP SimpleExpression notification rule plugin
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora
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

/**
 * Rule specific default configuration
 *
 * The "rule_config" property is a JSON object with asset name, 
 * list of datapoints used in expression and the boolean expression itself:
 *
 * Example:
    {
		"asset": {
			"description": "The asset name for which notifications will be generated.",
			"name": "modbus"
		},
		"datapoints": [{
			"type": "float",
			"name": "humidity"
		}, {
			"type": "float",
			"name": "temperature"
		}],
		"expression": {
			"description": "The expression to evaluate",
			"name": "Expression",
			"type": "string",
			"value": "if( humidity > 50, 1, 0)"
		}
	}
 
 * Expression is composed of datapoint values within given asset name.
 * And if the value of boolean expression toggles, then the notification is sent.
 */

#define xstr(s) str(s)
#define str(s) #s

#define DEF_RULE_CFG_VALUE  "{\"asset\":{\"description\":\"The asset name for which notifications will be generated.\",\"name\":\"modbus\"},\"datapoints\":[{\"type\":\"float\",\"name\":\"humidity\"},{\"type\":\"float\",\"name\":\"temperature\"}],\"expression\":{\"description\":\"The expression to evaluate\",\"name\":\"Expression\",\"type\":\"string\",\"value\":\"if( ((humidity > 50)), 1, 0)\"}}"

#define RULE_DEFAULT_CONFIG \
			"\"description\": { " \
				"\"description\": \"Generate a notification if all configured assets trigger\", " \
				"\"type\": \"string\", " \
				"\"default\": \"Generate a notification if all configured assets trigger\", " \
				"\"displayName\" : \"Rule\", " \
				"\"order\": \"1\" }, " \
			"\"rule_config\": { \"description\": \"The array of rules.\", \"type\": \"JSON\", \"default\": " xstr(DEF_RULE_CFG_VALUE) ", \"displayName\" : \"Configuration\", \"order\": \"2\" }, " \
			"\"asset\": { \"description\": \"The Asset name.\", \"type\": \"string\", " \
				"\"default\": \"Expression\", \"displayName\" : \"Asset\", \"order\": \"3\"}, " \
			"\"expression\": {\"description\": \"The expression to evaluate\", " \
				"\"name\": \"Expression\", \"type\": \"string\", \"default\": \"if( ((humidity > 50)), 1, 0)\", " \
				"\"displayName\" : \"The expression to evaluate\", \"order\": \"4\"}"

#define BUITIN_RULE_DESC "\"plugin\": {\"description\": \"" RULE_NAME " notification rule\", " \
			"\"type\": \"string\", \"default\": \"" RULE_NAME "\", \"readonly\": \"true\"}"

#define RULE_DEFAULT_CONFIG_INFO "{" BUITIN_RULE_DESC ", " RULE_DEFAULT_CONFIG "}"

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
	RULE_DEFAULT_CONFIG_INFO	// Configuration
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

	string ret = "{ \"reason\": \"";
	ret += rule->getState() == SimpleExpression::StateTriggered ? "triggered" : "cleared";
	ret += "\" }";

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
	bool assetEval = false;
	
	vector<Datapoint *> vec;

	for (auto & m : assetValue.GetObject())
	{
		if (m.value.IsDouble())
		{
			DatapointValue dpv((double) m.value.GetDouble());
			vec.emplace_back(new Datapoint(m.name.GetString(), dpv));
		}
		else if (m.value.IsNumber())
		{
			DatapointValue dpv((long) m.value.GetInt());
			vec.emplace_back(new Datapoint(m.name.GetString(), dpv));
		}
	}
	
	assetEval = m_triggerExpression->evaluate(vec);
	Logger::getLogger()->debug("m_triggerExpression->evaluate() returned assetEval=%s", assetEval?"true":"false");

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
	m_triggerExpression = NULL;
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
	string JSONrules = config.getValue("rule_config");
	string assetName =  config.getValue("asset");
	string expression =  config.getValue("expression");

	Document doc;
	doc.Parse(JSONrules.c_str());

	if (!doc.HasParseError())
	{
		// evaluation_type can be empty, it means latest value
		
		const Value& datapoints = doc["datapoints"];
		bool foundDatapoints = false;

		if (datapoints.IsArray())
		{
			vector<Datapoint *>	vec;
			for (auto & d : datapoints.GetArray())
			{
				if (d.HasMember("name"))
				{
					foundDatapoints = true;
					string dataPointName = d["name"].GetString();
					string datatype = d["type"].GetString();
					DatapointValue *dpv;
					if (datatype.compare("integer")==0)
						dpv = new DatapointValue(1L);
					else if (datatype.compare("float")==0)
						dpv = new DatapointValue(1.0f);
					else
					{
						Logger::getLogger()->info("Cannot handle datapoint: name=%s, type=%s, skipping...", dataPointName.c_str(), datatype.c_str());
						continue;
					}
					vec.emplace_back(new Datapoint(dataPointName, *dpv));
					//Logger::getLogger()->info("Added DP to vector= {%s : %s}", dataPointName.c_str(), dpv->toString().c_str());
					delete dpv;
				}
			}
			if (vec.size() <= 0)
			{
				Logger::getLogger()->info("Couldn't find any valid datapoint in expr rule plugin config");
				return false;
			}
			else
			{
				//Logger::getLogger()->info("Found %d datapoints in expr rule plugin config", vec.size());

				// Configuration change is protected by a lock
				this->lockConfig();

				try
				{
					m_triggerExpression = new Evaluator(vec, expression);
				}
				catch (...)
				{
					this->unlockConfig();
					Logger::getLogger()->error("SimpleExpression::configure() failed");
					return false;
				}

				if (this->hasTriggers())
				{       
					this->removeTriggers();
				}
				this->addTrigger(assetName, NULL);
				
				// Release lock
				this->unlockConfig();

				return true;
			}
		}
	}
	return false;
}

/**
 * Constructor for the evaluator class. This holds the expressions and
 * variable bindings used to execute the triggers.
 *
 * @param datapoints	Vector of datapoints of which expression is composed
 * @param expression	The expression to evaluate
 */
SimpleExpression::Evaluator::Evaluator(vector<Datapoint *> &datapoints, const string& expression) : m_varCount(0)
{
	for (auto & dp : datapoints)
	{
		DatapointValue& dpvalue = dp->getData();
		if (dpvalue.getType() == DatapointValue::T_INTEGER ||
				dpvalue.getType() == DatapointValue::T_FLOAT)
		{
			m_variableNames[m_varCount++] = dp->getName();
		}
		if (m_varCount == MAX_EXPRESSION_VARIABLES)
		{
			Logger::getLogger()->error("Too many datapoints in reading (>%d)", MAX_EXPRESSION_VARIABLES);
			break;
		}
	}

	for (int i = 0; i < m_varCount; i++)
	{
		m_variables[i] = NAN;
		m_symbolTable.add_variable(m_variableNames[i], m_variables[i]);
	}

	typedef exprtk::parser_error::type error_t;
	
	bool rv = true;
	rv = m_symbolTable.add_constants();
	if (rv == false)
	{
		Logger::getLogger()->error("m_symbolTable.add_constants() failed");
		throw new exception();
	}
	m_expression.register_symbol_table(m_symbolTable);
	rv=true;
	rv = m_parser.compile(expression.c_str(), m_expression);
	if (rv == false)
	{
		Logger::getLogger()->error("Failed to compile expression: Error: %s\tExpression: %s", m_parser.error().c_str(), expression.c_str());
		throw new exception();
	}
}

/**
 * Evaluate an expression using the reading provided and return true of false. 
 *
 * @param	datapoints	The datapoints in reading from which variables values are taken
 * @return	Bool result of evaluatign the expression
 */
bool SimpleExpression::Evaluator::evaluate(vector<Datapoint *>& datapoints)
{
	for (auto it = datapoints.begin(); it != datapoints.end(); it++)
	{
		string name = (*it)->getName();
		double value = 0.0;
		DatapointValue& dpvalue = (*it)->getData();
		if (dpvalue.getType() == DatapointValue::T_INTEGER)
		{
			value = dpvalue.toInt();
		}
		else if (dpvalue.getType() == DatapointValue::T_FLOAT)
		{	
			//double d = static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(100)));
			value = dpvalue.toDouble();
			//Logger::getLogger()->debug("SimpleExpression::Evaluator::evaluate(): name=%s, value=%lf", name.c_str(), value);
		}
		
		for (int i = 0; i < m_varCount; i++)
		{
			if (m_variableNames[i].compare(name) == 0)
			{
				m_variables[i] = value;
				break;
			}
		}
	}
	Logger::getLogger()->debug("SimpleExpression::Evaluator::evaluate(): m_expression.value()=%lf", m_expression.value());
	if (std::isnan(m_expression.value()))
		Logger::getLogger()->error("SimpleExpression::Evaluator::evaluate(): unable to evaluate expression");
	return (m_expression.value() == 1.0);
}

