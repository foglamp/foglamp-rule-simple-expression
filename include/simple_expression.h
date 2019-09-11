#ifndef _SIMPLE_EXPRESSION_RULE_H
#define _SIMPLE_EXPRESSION_RULE_H
/*
 * Fledge SimpleExpression class
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora, Massimiliano Pinto
 */
#include <plugin.h>
#include <config_category.h>
#include <rule_plugin.h>
#include <builtin_rule.h>
#include <exprtk.hpp>

class Datapoint;

#define MAX_EXPRESSION_VARIABLES 20

/**
 * SimpleExpression class, derived from Notification BuiltinRule
 */
class SimpleExpression: public BuiltinRule
{
	public:
		class Evaluator {
			public:
				Evaluator();
				bool	parserCompile(const std::string& expression)
				{
					return m_parser.compile(expression.c_str(),
								m_expression);
				};
				std::string
					getError()
				{
					return m_parser.error();
				};
				void registerSymbolTable()
				{
					m_expression.register_symbol_table(m_symbolTable);
				};

				void	addVariable(const std::string& dapointName, double value);
				int	getVarCount() { return m_varCount; };
				double	evaluate() { return m_expression.value(); };

			private:
				exprtk::expression<double>	m_expression;
				exprtk::symbol_table<double>	m_symbolTable;
				exprtk::parser<double>		m_parser;
				double				m_variables[MAX_EXPRESSION_VARIABLES];
				std::string			m_variableNames[MAX_EXPRESSION_VARIABLES];
				int				m_varCount;
		};
	public:
		SimpleExpression();
		~SimpleExpression();

		bool	configure(const ConfigCategory& config);
		bool	evalAsset(const Value& assetValue);
		void	lockConfig() { m_configMutex.lock(); };
		void	unlockConfig() { m_configMutex.unlock(); };

		void	setTrigger(const std::string& expression)
			{
				m_expression = expression;
			};
		const std::string&
			getTrigger() { return m_expression; };

		Evaluator*
			getEvaluator() { return m_triggerExpression; };

	private:
		std::mutex	m_configMutex;
		std::string	m_expression;
		bool		m_pendingReconfigure;
		Evaluator	*m_triggerExpression;
};

#endif
