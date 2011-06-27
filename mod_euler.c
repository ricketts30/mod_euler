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

struct list_el {
	int val;
	char title[250];
	void (*execute)(request_rec *r);
	struct list_el * next;
	struct list_el * prev;
};

typedef struct list_el item;

int AddNewItem(item ** first, int eulerNumber, void (* execute)(request_rec *r), char *title); 
void Setup(item ** first);
void ListThemAll(item ** first, request_rec *r);
void RunTheProblem(item ** first, int number, request_rec *r);

void problem_1(request_rec *r);
void problem_2(request_rec *r);
void problem_3(request_rec *r);
void problem_4(request_rec *r);

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
	
	first = NULL;
		
	if (strcmp(r->handler, "euler-handler")) {
		// this is not mine to handle
		// although the handler must be set in the config
        return DECLINED;
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
	NULL,
	NULL,
	NULL,
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
		ap_rprintf(r, "<li><p><a href=\"%s?%d\">Problem No. %d</a></p></li> \n", r->uri, theNumber, theNumber);
		curr = curr->next;
	}
	ap_rputs("</ul>\n", r);
}

// EULER PROBLEM IMPLEMENTATIONS

void problem_1(request_rec *r){
	ap_rputs("<p>I</p>\n", r);
	ap_rprintf(r, "<p>This is problem one (%d)</p>\n", 1);
}

void problem_2(request_rec *r){
	ap_rputs("<p>II</p>\n", r);
	ap_rprintf(r, "<p>This is problem two (%d)</p>\n", 2);
}

void problem_3(request_rec *r){
	ap_rputs("<p>III</p>\n", r);
	ap_rprintf(r, "<p>This is problem three (%d)</p>\n", 3);
}

void problem_4(request_rec *r){
	ap_rputs("<p>IV</p>\n", r);
	ap_rprintf(r, "<p>This is problem four (%d)</p>\n", 4);
}

