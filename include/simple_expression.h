#ifndef _SIMPLE_EXPRESSION_RULE_H
#define _SIMPLE_EXPRESSION_RULE_H
/*
 * FogLAMP SimpleExpression class
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora
 */
#include <plugin.h>
#include <plugin_manager.h>
#include <config_category.h>
#include <rule_plugin.h>
#include <builtin_rule.h>
#include <exprtk.hpp>
#include <datapoint.h>

#define MAX_EXPRESSION_VARIABLES 20

/**
 * SimpleExpression class, derived from Notification BuiltinRule
 */
class SimpleExpression: public BuiltinRule
{
	public:
		SimpleExpression();
		~SimpleExpression();

		void	configure(const ConfigCategory& config);
		bool	evalAsset(const Value& assetValue, RuleTrigger* rule);
		void	lockConfig() { m_configMutex.lock(); };
		void	unlockConfig() { m_configMutex.unlock(); };

		void	setTrigger(const std::string& expression)
			{
				m_trigger = expression;
			};

	private:
		std::mutex	m_configMutex;
		
		class Evaluator {
			public:
				Evaluator(std::vector<Datapoint *> &datapoints, const std::string& expression);
				bool		evaluate(std::vector<Datapoint *>& datapoints);
			private:
				exprtk::expression<double>	m_expression;
				exprtk::symbol_table<double>	m_symbolTable;
				exprtk::parser<double>		m_parser;
				double				m_variables[MAX_EXPRESSION_VARIABLES];
				std::string			m_variableNames[MAX_EXPRESSION_VARIABLES];
				int				m_varCount;
		};
		std::string		m_trigger;
		bool			m_pendingReconfigure;
		Evaluator		*m_triggerExpression;
};

#endif
