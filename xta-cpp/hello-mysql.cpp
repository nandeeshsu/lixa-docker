/*
 * Copyright (c) 2009-2018, Christian Ferrari <tiian@users.sourceforge.net>
 * All rights reserved.
 *
 * This file is part of LIXA.
 *
 * LIXA is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * LIXA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LIXA.  If not, see <http://www.gnu.org/licenses/>.
 */



/*
 * This program is an example implementation of the
 * "Single Application" Pattern
 * as documented in LIXA manual:
 * http://www.tiian.org/lixa/manuals/html/index.html
 *
 * This program accepts exactly three parameters on the command line:
 * first parameter: hostname for MySQL server
 * second parameter:  "commit", boolean value (if FALSE, "rollback")
 * third parameter: "insert", boolean value (if FALSE, "delete")
 *
 * Programming Style note:
 * the purpose of this small program is not to explain C++ development
 * techniques or good style, but simply to show XTA for C++ using the easiest
 * approach.
 * "xta" namespace is explicitly put in every statement (see "xta::") but it
 * can be avoided with "using namespace xta;"
 */



// Standard headers
#include <iostream>
// This header is necessary for all the stuff related to XTA
#include <xta/cpp/Xta.hpp>



int main(int argc, char *argv[])
{
    /* First parameter: MySQL server hostname */
    const char                   *hostname;
    /* Second parameter: commit transaction? */
    int                           commit;
    /* Third parameter: insert data in databases? */
    int                           insert;
    /* native MySQL connection handler */
    MYSQL                        *rm2 = NULL;
    /* variable for MySQL statement to execute */
    const char                   *mysql_stmt;
    /* XTA Resource for MySQL */
    xta::MysqlXaResource         *xar2 = NULL;
    /* XTA Transaction Manager object reference */
    xta::TransactionManager      *tm = NULL;
    /*
     * Check command line parameters
     */
    if (argc < 4) {
        cerr << "This program requires MySQL hostname and "
                "two boolean parameters: 'commit' and 'insert'\n";
        return 1;
    }
    hostname = argv[1];
    commit = strtol(argv[2], NULL, 0);
    insert = strtol(argv[3], NULL, 0);
    /*
     * Prepare SQL statements in accordance with "insert" command line
     * parameter
     */
    if (insert) {
        mysql_stmt = "INSERT INTO authors VALUES(1919, 'Levi', 'Primo')";
    } else {
        mysql_stmt = "DELETE FROM authors WHERE id=1919";
    }

    /*
     * initialize XTA environment
     */
    xta::Xta::init();
    /*
     * create a new MySQL connection
     * Note: using MySQL C API and C standard functions
     */
    rm2 = mysql_init(NULL);
    if (rm2 == NULL) {
        fprintf(stderr, "mysql_init: returned NULL\n");
        return 1;
    }
    if (mysql_real_connect(rm2, hostname, "lixa", "passw0rd",
                           "lixa", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect: returned error: %u, %s\n",
                mysql_errno(rm2), mysql_error(rm2));
        return 1;
    }
    /*
     * create XTA objects necessary to start a transaction
     */
    try {
        // create a new XTA Transaction Manager object
        tm = new xta::TransactionManager();
        // create an XA resource for MySQL
        // second parameter "MySQL" is descriptive
        // third parameter "localhost,0,lixa,,lixa" identifies the specific
        // database
        xar2 = new xta::MysqlXaResource(
            rm2, "MySQL", "localhost,0,lixa,,lixa");
        // Create a new XA global transaction and retrieve a reference from
        // the TransactionManager object
        xta::Transaction tx = tm->createTransaction();
        // Enlist MySQL resource to Transaction
        tx.enlistResource(xar2);
        // Start a new XA global transaction with a single branch
        tx.start();
        /*
         * At this point, it's time to do something with the Resource Manager
         *
         * Execute MySQL statement
         */
        printf("MySQL, executing >%s<\n", mysql_stmt);
        if (mysql_query(rm2, mysql_stmt)) {
            fprintf(stderr, "MySQL, error while executing >%s<: %u/%s\n",
                    mysql_stmt, mysql_errno(rm2), mysql_error(rm2));
            mysql_close(rm2);
            return 1;
        }
        /*
         * commit or rollback the transaction
         */
        if (commit) {
            tx.commit();
        } else {
            tx.rollback();
        }
        /*
         * Delete MySQL native and XA resource
         */
        delete xar2;
        /*
         * Close the MySQL connection
         */
        mysql_close(rm2);
        /*
         * Delete Transaction Manager object
         */
        delete tm;
        
    } catch (xta::Exception e) {
        /*
         * what() is a standard method that describes the exception
         * where() is a method provided by XTA to describe the XTA C function
         *         that raised the exception
         * getReturnCode() is a method provided by XTA to retrieve the
         *                 integer reason code returned by XTA C function
         *                 (see file lixa_errors.h)
         */
        cerr << "Exception in function '" << e.where() <<
            "', return code description: '" << e.what() << "', " <<
            "return code: " << e.getReturnCode() << endl;
        return 1;
    }
    
    return 0;
}

