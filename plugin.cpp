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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <logger.h>
#include <plugin_exception.h>
#include <iostream>
#include <config_category.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
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

#define DEF_RULE_CFG_VALUE  "{\"asset\":{\"description\":\"The asset name for which notifications will be generated.\",\"name\":\"modbus\"},\"datapoints\":[{\"type\":\"float\",\"name\":\"humidity\"},{\"type\":\"float\",\"name\":\"temperature\"}],\"expression\":{\"description\":\"The expression to evaluate\",\"name\":\"Expression\",\"type\":\"string\",\"value\":\"if( humidity > 50, 1, 0)\"}}"

#define RULE_DEFAULT_CONFIG \
			"\"description\": { " \
				"\"description\": \"Generate a notification if all configured assets trigger\", " \
				"\"type\": \"string\", " \
				"\"default\": \"Generate a notification if all configured assets trigger\", " \
				"\"displayName\" : \"Rule\", " \
				"\"order\": \"1\" }, " \
			"\"rule_config\": { \"description\": \"The array of rules.\", \"type\": \"JSON\", \"default\": " xstr(DEF_RULE_CFG_VALUE) ", \"displayName\" : \"Configuration\", \"order\": \"2\" }"

#define BUITIN_RULE_DESC "\"plugin\": {\"description\": \"" RULE_NAME " notification rule\", " \
			"\"type\": \"string\", \"default\": \"" RULE_NAME "\", \"readonly\": \"true\"}"

#define RULE_DEFAULT_CONFIG_INFO "{" BUITIN_RULE_DESC ", " RULE_DEFAULT_CONFIG "}"

using namespace std;

bool evalAsset(const Value& assetValue, RuleTrigger* rule);

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
	PRINT_FUNC;
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
	PRINT_FUNC;
	SimpleExpression* handle = new SimpleExpression();
	handle->configure(config);

	return (PLUGIN_HANDLE)handle;
}

/**
 * Free rule resources
 */
void plugin_shutdown(PLUGIN_HANDLE handle)
{
	PRINT_FUNC;
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
	PRINT_FUNC;
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
		
#if 0
		if (!(*it).second->getEvaluation().empty())
		{
			ret += ", \"" + (*it).second->getEvaluation() + "\" : " + \
				to_string((*it).second->getInterval()) + " }";
		}
		else
		{
			ret += " }";
		}
#endif
		
		if (std::next(it, 1) != triggers.end())
		{
			ret += ", ";
		}
	}

	ret += " ] }";

	// Release lock
	rule->unlockConfig();

	PRINT_FUNC;
	Logger::getLogger()->info("plugin_triggers(): ret=%s", ret.c_str());

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
	//Logger::getLogger()->info("plugin_eval(): assetValues=%s", assetValues.c_str());
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
			eval = rule->evalAsset(assetValue, NULL);
			Logger::getLogger()->info("plugin_eval(): eval=%s", eval?"true":"false");
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

	Logger::getLogger()->info("plugin_reason(): ret=%s", ret.c_str());

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
	PRINT_FUNC;
	SimpleExpression* rule = (SimpleExpression *)handle;
	ConfigCategory  config("newCfg", newConfig);
	rule->configure(config);
}

// End of extern "C"
};

#if 0
/**
 * Check whether the input datapoint
 * is a NUMBER and its value is greater than configured DOUBLE limit
 *
 * @param    point		Current input datapoint
 * @param    limitValue		The DOUBLE limit value
 * @return			True if limit is hit,
 *				false otherwise
 */
bool checkDoubleLimit(const Value& point, double limitValue)
{
	bool ret = false;

	// Check config datapoint type
	if (point.GetType() == kNumberType)
	{
		if (point.IsDouble())
		{       
			if (point.GetDouble() > limitValue)
			{       
				ret = true;
			}
		}
		else
		{
  			if (point.IsInt() ||
			    point.IsUint() ||
			    point.IsInt64() ||
			    point.IsUint64())
			{
				if (point.IsInt() ||
				    point.IsUint())
				{
					if (point.GetInt() > limitValue)
					{
						ret = true;
					}
				}
				else
				{
					if (point.GetInt64() > limitValue)
					{
						ret = true;
					}
				}
			}
		}
	}

	return ret;
}
#endif

/**
 * Evaluate datapoints values for the given asset name
 *
 * @param    assetValue		JSON object with datapoints
 * @param    rule		Current configured rule trigger.
 *
 * @return			True if evalution succeded,
 *				false otherwise.
 */
bool SimpleExpression::evalAsset(const Value& assetValue, RuleTrigger* rule)
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
	//Logger::getLogger()->info("m_triggerExpression->evaluate() returned assetEval=%s", assetEval?"true":"false");

#if 0
	bool evalAlldatapoints = rule->evalAllDatapoints();
	// Check all configured datapoints for current assetName
	vector<Datapoint *> datapoints = rule->getDatapoints();
	for (auto & it : datapoints)
	{
		string datapointName = (*it)->getName();
		// Get input datapoint name
		if (assetValue.HasMember(datapointName.c_str()))
		{
			const Value& point = assetValue[datapointName.c_str()];
			// Check configuration datapoint type
			switch ((*it)->getData().getType())
			{
			case DatapointValue::T_FLOAT:
				assetEval = checkDoubleLimit(point,
							   (*it)->getData().toDouble());
				break;
			case DatapointValue::T_STRING:
			default:
				break;
				assetEval = false;
			}

			// Check eval all datapoints
			if (assetEval == true &&
			    evalAlldatapoints == false)
			{
				// At least one datapoint has been evaluated
				break;
			}
		}
		else
		{
			assetEval = false;
		}
	}
#endif

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
}

/**
 * SimpleExpression destructor
 */
SimpleExpression::~SimpleExpression()
{
}

/**
 * Configure the rule plugin
 *
 * @param    config	The configuration object to process
 */
void SimpleExpression::configure(const ConfigCategory& config)
{
	PRINT_FUNC;
	string JSONrules = config.getValue("rule_config");
	//string JSONrules("{\"asset\":{\"description\":\"The asset name for which notifications will be generated.\",\"name\":\"humidtemp\"},\"datapoints\":[{\"type\":\"float\",\"name\":\"humidity\"},{\"type\":\"float\",\"name\":\"temperature\"}],\"expression\":{\"description\":\"The expression to evaluate\",\"name\":\"Expression\",\"type\":\"string\",\"value\":\"clamp(-1,sin(2 * pi * humidity) + cos(temperature / 2 * pi),+1)\"}    }");
	Logger::getLogger()->info("JSONrules=%s", JSONrules.c_str());

	Document doc;
	doc.Parse(JSONrules.c_str());

	if (!doc.HasParseError())
	{
		if (!doc.HasMember("asset") || !doc.HasMember("datapoints") || !doc.HasMember("expression"))
		{
			return;
		}
		PRINT_FUNC;

		const Value& asset = doc["asset"];
		string assetName = asset["name"].GetString();
		if (assetName.empty())
		{
			return;
		}
		PRINT_FUNC;

		const Value& exprVal = doc["expression"];
		string expr = exprVal["value"].GetString();
		if (expr.empty())
		{
			return;
		}
		PRINT_FUNC;
		
		// evaluation_type can be empty, it means latest value

		// Configuration change is protected by a lock
		this->lockConfig();
		if (this->hasTriggers())
		{       
			this->removeTriggers();
		}
		this->addTrigger(assetName, NULL);
		Logger::getLogger()->info("Added trigger: assetName=%s", assetName.c_str());
		// Release lock
		this->unlockConfig();

		PRINT_FUNC;
		
		const Value& datapoints = doc["datapoints"];
		bool evalAlldatapoints = true;
		bool foundDatapoints = false;
		if (doc.HasMember("eval_all_datapoints") &&
		    doc["eval_all_datapoints"].IsBool())
		{
			evalAlldatapoints = doc["eval_all_datapoints"].GetBool();
		}

		if (datapoints.IsArray())
		{
			PRINT_FUNC;
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
						Logger::getLogger()->info("Cannot handle datapoint: name=%s, type=%s, skipping...", dataPointName, datatype);
						continue;
					}
					vec.emplace_back(new Datapoint(dataPointName, *dpv));
					Logger::getLogger()->info("Added DP to vector= {%s : %s}", dataPointName.c_str(), dpv->toString().c_str());
					delete dpv;
				}
			}
			if (vec.size() <= 0)
			{
				Logger::getLogger()->info("Cannot find any valid datapoint");
				return;
			}
			else
				Logger::getLogger()->info("vec.size()=%d", vec.size());

			// TODO: take care of config change mutex in all 'return' cases

			m_triggerExpression = new Evaluator(vec, expr);

			
		}
		if (!foundDatapoints)
		{
			// Log message
		}
	}
}

/**
 * Constructor for the evaluator class. This holds the expressions and
 * variable bindings used to execute the triggers.
 *
 * @param reading	An initial reading to use to create varaibles
 * @parsm expression	The expression to evaluate
 */
SimpleExpression::Evaluator::Evaluator(vector<Datapoint *> &datapoints, const string& expression) : m_varCount(0)
{
	//vector<Datapoint *>	datapoints = reading->getReadingData();
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
			Logger::getLogger()->error("Too many datapoints in reading");
			break;
		}
	}

	for (int i = 0; i < m_varCount; i++)
	{
		m_symbolTable.add_variable(m_variableNames[i], m_variables[i]);
	}

	m_symbolTable.add_constants();
	m_expression.register_symbol_table(m_symbolTable);
	m_parser.compile(expression.c_str(), m_expression);

}

/**
 * Evaluate an expression using the reading provided and return true of false. 
 *
 * @param	reading	The reading from which the variables are taken
 * @return	Bool result of evaluatign the expression
 */
bool SimpleExpression::Evaluator::evaluate(vector<Datapoint *>& datapoints)
{
	//vector<Datapoint *> datapoints = reading->getReadingData();
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
			double d = static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(100)));
			value = dpvalue.toDouble() + d;
			Logger::getLogger()->info("SimpleExpression::Evaluator::evaluate(): name=%s, value=%lf", name.c_str(), value);
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
	//Logger::getLogger()->info("SimpleExpression::Evaluator::evaluate(): m_expression.value()=%lf", m_expression.value());
	return m_expression.value() != 0.0;
}

