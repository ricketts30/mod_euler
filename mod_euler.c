/*
	filename: mod_euler.c

	description: a small Apache module
	
	copyright: Copyright 2011 http://predication.posterous.com

	This program is distributed in the hope that it will be useful
	or educational,	but WITHOUT ANY WARRANTY; without even the 
	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
	PURPOSE.

*/

#include <stdio.h>
#include <stdlib.h>

/*
 * Include the core server components.
 */
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "http_connection.h"

#include "apr_strings.h"

/* this module */
module AP_MODULE_DECLARE_DATA euler_module;

struct list_el {
	int val;
	char title[250];
	void (*execute)(request_rec *r);
	struct list_el * next;
	struct list_el * prev;
};

typedef struct list_el item;

// this is a struct to hold configuration stuff
// the things that we would be configuring are
// the location of the style sheet
// EulerStylesheetFilePath string char[250]

typedef struct {
	char *eulerStylesheetFilePath;
} modEuler_config;

#ifndef DEFAULT_EULER_STYLESHEET_FILE_PATH
#define DEFAULT_EULER_STYLESHEET_FILE_PATH "/etc/apache2/euler.config"
#endif

#ifndef DEFAULT_EULER_LINE_LENGTH
#define DEFAULT_EULER_LINE_LENGTH 100
#endif



int AddNewItem(item ** first, int eulerNumber, void (* execute)(request_rec *r), char *title); 
void Setup(item ** first);
void ListThemAll(item ** first, request_rec *r);
void RunTheProblem(item ** first, int number, request_rec *r);

void problem_1(request_rec *r);
void problem_2(request_rec *r);
void problem_3(request_rec *r);
void problem_4(request_rec *r);
void problem_5(request_rec *r);
void problem_6(request_rec *r);

/*
 * This is the entry point for processing and is invoked on GET requests.
 * If you do a 
 *    printf(stderr, "This is a log message.\n");
 * then it will appear in the apache error_log file
 */
static int mod_euler_method_handler (request_rec *r)
{
	// this is either a number greater than zero (indicating that a problem number was selected)
	// or it is zero or less (indicating that no problem was selected)
	int eulerProblemNumber;
	item * first;
	char line[DEFAULT_EULER_LINE_LENGTH];
	modEuler_config *s_cfg;
	// used for reading the stylesheet
	FILE *fr;
	first = NULL;
	
	// Get the module configuration
	s_cfg = ap_get_module_config(r->server->module_config, &euler_module);
	
	// Send a message to the log file.
	// [thanks to Min Xu for the security suggestion]
	fprintf(stderr,"%s",s_cfg->eulerStylesheetFilePath);
		
	if (strcmp(r->handler, "euler-handler")) {
		// this is not mine to handle
		// although the handler must be set in the config
        return DECLINED;
    }
	
	// check this is a request for the stylesheet

	if(r->args != NULL && 0 == strcmp(r->args, "css")){
		ap_set_content_type(r, "text/css");
		fr = fopen(s_cfg->eulerStylesheetFilePath, "r");
		if(NULL == fr){
			// there isn't a file so return a dummy one
			ap_rputs("/* mod_euler.css */ \n", r);
			ap_rputs("/* normal file was missing */ \n", r);
			ap_rputs("body { font-family: monospace; } \n", r);
			ap_rputs("h1 { color: navy; } \n", r);
		} else {
			while(fgets(line, DEFAULT_EULER_LINE_LENGTH, fr) != NULL)
			{
				ap_rprintf(r, "%s", line);
			}
		}
		fclose(fr);
		return OK;
	}

	// important to set the mime-type
	ap_set_content_type(r, "text/html");
	
    if (r->header_only) {
    	// don't need to do more for a head only request
        return OK;
    }
	
	Setup(&first);
	
	// do some output
	// the standard HTML5 doctype
    ap_rputs("<!DOCTYPE html>\n", r);
	ap_rputs("<html lang=\"en\">\n", r);
	
	// get the stylesheet
	ap_rprintf(r, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s?css\" /> \n", r->uri);
	
	ap_rputs("<head>\n", r);
	ap_rputs("<meta charset=\"utf-8\" />\n", r);
	
	// we might need to check if this is 
	// the main page or a problem  
	eulerProblemNumber = 0;
	if(r->args != NULL){
		eulerProblemNumber = atoi(r->args);
	}
	
	if(eulerProblemNumber > 0){
		ap_rprintf(r, "<title>Euler Problem Number %d</title>\n", eulerProblemNumber);
	} else {
		ap_rputs("<title>Project Euler Web App</title>\n", r);
	}
	
	ap_rputs("</head>\n", r);
	ap_rputs("</body>\n", r);
	
	if(eulerProblemNumber > 0){
		ap_rprintf(r, "<h1>Euler Problem Number %d</h1>\n", eulerProblemNumber);
		// now put a link back to the main page
		ap_rprintf(r, "<p><a href=\"%s\">Back<a> to list of problems</p>\n", r->uri);
		// now run the problem
		RunTheProblem(&first, eulerProblemNumber, r);
	} else {
		ap_rprintf(r, "<h1>%s</h1>\n", "Euler Problems");
		// now print a list of all the available problems
		ListThemAll(&first, r);
	}
	
	ap_rputs("</body>\n", r);
	ap_rputs("</html>\n", r);
    /*
     * We did what we wanted to do, so tell the rest of the server we
     * succeeded.
     */
    return OK; // DECLINED, DONE, OK
}

// this is the hook to fire when creating the configuration
static void *create_mod_euler_config(apr_pool_t *p, server_rec *s){
	// the config record
	modEuler_config *theConfig;
	// allocate memory for it (
	// Allocate memory from the provided pool.
	theConfig = (modEuler_config *) apr_pcalloc(p, sizeof(modEuler_config));

	theConfig->eulerStylesheetFilePath = DEFAULT_EULER_STYLESHEET_FILE_PATH;
	
	// Set the string to a default value.
	//theConfig->eulerStylesheetFilePath = DEFAULT_EULER_STYLESHEET_FILE_PATH;
	
	// Return the created configuration struct.
	// casting to the void pointer is a bit like returning an 'object'
	return (void *) theConfig;
	//return NULL;
}

static const char *set_mod_euler_string(cmd_parms *parms, void *mconfig, const char *arg)
{
	modEuler_config *s_cfg = ap_get_module_config(parms->server->module_config, &euler_module);
	s_cfg->eulerStylesheetFilePath = (char *) arg;
	// if OK return a NULL
	// if a problem then return a string here an apache will pass it to the user
	return NULL;
}

// this is an array: a list of the module settigs to pick up and process
// in the httpd.conf file
static const command_rec mod_euler_cmds[] =
{
	AP_INIT_TAKE1(
			  "EulerStylesheetFilePath",
			  set_mod_euler_string,
			  NULL,
			  RSRC_CONF,
			  "MOD_EULER :: Problem with the config item - EulerStylesheetFilePath"
			  ),
	{NULL}
};

// this function handles registering the various hooks
static void mod_euler_register_hooks (apr_pool_t *p)
{
	ap_hook_handler(mod_euler_method_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/*
 * This is the module's main data structure.  
 * Note: the name of this structure must match the name of the module.
 */
module AP_MODULE_DECLARE_DATA euler_module =
{
	// currently this is a very minimal module
	// it has no configuration, startup, authentication, or teardown code
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	create_mod_euler_config,
	NULL,
	mod_euler_cmds,
	// the callback function for registering hooks
	mod_euler_register_hooks,
};



//////////////////////////////////////////////////////////////
/// Problem Implementations and Setup ////////////////////////
//////////////////////////////////////////////////////////////

// SETUP

void Setup(item ** first){
	int rv_AddNewItem;
	rv_AddNewItem = AddNewItem(first, 1, problem_1, "Add all the natural numbers below one thousand that are multiples of 3 or 5.");
	rv_AddNewItem = AddNewItem(first, 2, problem_2, "By considering the terms in the Fibonacci sequence whose values do not exceed four million, find the sum of the even-valued terms.");
	
	rv_AddNewItem = AddNewItem(first, 5, problem_5, "What is the smallest number divisible by each of the numbers 1 to 20?");
	rv_AddNewItem = AddNewItem(first, 6, problem_6, "Find the difference between the sum of the squares of the first one hundred natural numbers and the square of the sum.");
}

// they are just appended to the end with no duplicate checking.
int AddNewItem(item ** first, int eulerNumber, void (* execute)(request_rec *r), char * title){
	int retValue; // this indicates that everything is OK [0 is OK, -1 = already in the list, -2 = other problem occurred]
	// initially we just insert at the end
	item * curr, * previous;
	curr = * first;
	previous = NULL;
	// now for the body of code...
	retValue = 0;
	while(curr){
		previous = curr;
		curr = curr->next;
	}
	// we have the latest one as curr
	curr = (item *)malloc(sizeof(item));
	curr->val = eulerNumber;
	curr->execute = execute;
	curr->prev = previous;
	curr->next = NULL;
	// this is safe
	strncpy(curr->title, title,250);
	curr->title[250] = '\0';
	// if this is the first then hook it up
	if(previous == NULL){
		// this updates the pointed-to-value?
		*first = curr;
	} else {
		// update the previous one's "next" pointer
		previous->next = curr;
	}
	return retValue;
}

void RunTheProblem(item ** first, int number, request_rec *r){
	// we loop through all of them
	item * curr;
	int theNumber;
	curr = *first;
	theNumber = 0;
	while(curr){
		if(curr->val == number){
			ap_rprintf(r, "<h2>%s</h2> \n", curr->title);
			ap_rprintf(r, "<p><a href=\"http://projecteuler.net/index.php?section=problems&id=%d\">Link to ProblemEuler.net</a></p> \n", number);
			curr->execute(r);
			return;
		}
		curr = curr->next;
	}
}

void ListThemAll(item ** first, request_rec *r){
	// we loop through all of them
	item * curr;
	int theNumber;
	curr = *first;
	theNumber = 0;
	ap_rputs("<ul>\n", r);
	while(curr){
		theNumber = curr->val;
		ap_rprintf(r, "<li><p><a href=\"%s?%d\">Problem No. %d</a> - %s</p></li> \n", r->uri, theNumber, theNumber, curr->title);
		curr = curr->next;
	}
	ap_rputs("</ul>\n", r);
}

// EULER PROBLEM IMPLEMENTATIONS

void problem_1(request_rec *r){
	// If we list all the natural numbers below 10 that are multiples of 3 or 5, we get 3, 5, 6 and 9. The sum of these multiples is 23.
	// Find the sum of all the multiples of 3 or 5 below 1000.
	int sum = 0;
	int looper =0 ;
	for(looper = 0; looper < 1000; looper++)
		if(looper % 3 == 0 || looper % 5 == 0)
			sum+=looper;
	
	ap_rprintf(r, "<p>The sum of all the natural multiples of 3 or 5 (below 1000) is <em>%d</em></p>\n", sum);
}

void problem_2(request_rec *r){
	// Each new term in the Fibonacci sequence is generated by adding the previous two terms. By starting with 1 and 2, the first 10 terms will be:
	// 1, 2, 3, 5, 8, 13, 21, 34, 55, 89
	// By considering the terms in the Fibonacci sequence whose values do not exceed four million, find the sum of the even-valued terms.
	long previous = 1;
	long current = 2;
	long next = 0;
	long sum = 0;
	while(current < 4000000){
		if(current % 2 == 0)
			sum += current;
		next = previous + current;
		previous = current;
		current = next;
	}
	ap_rprintf(r, "<p>The sum of all the even fibonacci numbers less than 4 million is <em>%d</em></p>\n", sum);
}

void problem_3(request_rec *r){
	ap_rputs("<p>Not yet implemented.</p>\n", r);
}

void problem_4(request_rec *r){
	ap_rputs("<p>Not yet implemented.</p>\n", r);
}

// TODO - add a timer to allow us to compare approaches...
// this version takes a very long time...
void problem_5(request_rec *r){
	// 2520 is the smallest number that can be divided by each of the numbers from 1 to 10 without any remainder.
	// What is the smallest positive number that is evenly divisible by all of the numbers from 1 to 20?
	long smallest_number = 1;
	long counter = 1;
	unsigned char is_found = 0;
	while (is_found == 0 && smallest_number < 40000000000) {
		is_found = 1;
		for(counter = 1; counter < 21; counter++) {
			if(smallest_number % counter != 0) {
				is_found = 0;
			}
		}
		if(is_found == 0)
			smallest_number++;
	}
	if(is_found != 0)
		ap_rprintf(r, "<p>The smallest positive number that is evenly divisible by all of the numbers from 1 to 20 is <em>%d</em></p>\n", smallest_number);
	else
		ap_rputs("<p>The smallest positive number that is entirely divisble by all the numbers from 1 to 20 is not found to be under 40 million!</p>\n", r);
}

void problem_6(request_rec *r){
	// The sum of the squares of the first ten natural numbers is,
	//    1^2 + 2^2 + ... + 10^2 = 385
	// The square of the sum of the first ten natural numbers is,
	//   (1 + 2 + ... + 10)^2 = 552 = 3025
	// Hence the difference between the sum of the squares of the first ten natural numbers and the square of the sum is 3025  385 = 2640.
	// Find the difference between the sum of the squares of the first one hundred natural numbers and the square of the sum.
	long sum_of_squares = 0;
	long square_of_sum = 0;
	long counter = 0;
	for(counter = 1; counter < 101; counter++){
		sum_of_squares += (counter * counter); 
		square_of_sum += counter;
	}
	square_of_sum *= square_of_sum;
	ap_rprintf(r, "<p>The difference between the sum of the squares of the first one hundred natural numbers and the square of the sum is <em>%d</em></p>\n", (square_of_sum - sum_of_squares));
}

