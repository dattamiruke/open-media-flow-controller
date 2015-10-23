/*
 *
 * Filename:    md_agentd_pm.c
 * Date:        2011-11-08
 * Author:      Sobia Preethi
 *
 * (C) Copyright 2008 Nokeena Networks, Inc.
 * All rights reserved/
 *
 */

/*----------------------------------------------------------------------------
 * md_agentd_pm.c: shows how the nkn_agentd module is added to
 * the initial PM database.
 *
 * This module does not register any nodes of its own. Its sole purpose
 * is to add/change nodes in the initial database.
 *---------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
 * HEADER FILES
 *---------------------------------------------------------------------------*/
#include "common.h"
#include "dso.h"
#include "md_mod_reg.h"
#include "md_upgrade.h"
#include "nkn_mgmt_common.h"

/*----------------------------------------------------------------------------
 * PREPROCESSOR MACROS/CONSTANTS
 *---------------------------------------------------------------------------*/
#define AGENTD_PATH          "/opt/nkn/sbin/agentd"
#define AGENTD_WORKING_DIR   "/coredump/agentd"


/*----------------------------------------------------------------------------
 * PRIVATE VARIABLE DECLARATIONS
 *---------------------------------------------------------------------------*/
static md_upgrade_rule_array *md_agentd_pm_ug_rules = NULL;

static const bn_str_value md_agentd_pm_initial_values[] = {
        /*
         * agentd : basic launch configuration.
         */
        {"/pm/process/agentd/launch_enabled", bt_bool, "true"},
	/* this should not start on boot */
        {"/pm/process/agentd/auto_launch", bt_bool, "false"},
        {"/pm/process/agentd/working_dir", bt_string, AGENTD_WORKING_DIR},
        {"/pm/process/agentd/launch_path", bt_string, AGENTD_PATH},
        {"/pm/process/agentd/launch_uid", bt_uint16, "0"},
        {"/pm/process/agentd/launch_gid", bt_uint16, "0"},
        {"/pm/process/agentd/initial_launch_order", bt_int32, NKN_LAUNCH_ORDER_AGENTD},
        {"/pm/process/agentd/initial_launch_timeout", bt_duration_ms, NKN_LAUNCH_TIMEOUT_INGESTER},


};

/*----------------------------------------------------------------------------
 * FUNCTION DECLARATIONS
 *---------------------------------------------------------------------------*/
int md_agentd_pm_init(const lc_dso_info *info, void *data);


static int md_nkn_pm_add_initial(md_commit *commit, mdb_db *db, void *arg)
{
        int err = 0;

        err = mdb_create_node_bindings(
                        commit,
                        db,
                        md_agentd_pm_initial_values,
                        sizeof(md_agentd_pm_initial_values)/ sizeof(bn_str_value));
        bail_error(err);

bail:
        return err;
}

int
md_agentd_pm_init(const lc_dso_info *info, void *data)
{
    int err = 0;
    md_module *module = NULL;
    md_reg_node *node = NULL;
    md_upgrade_rule *rule = NULL;
    md_upgrade_rule_array *ra = NULL;

    /*
     * Commit order of 200 is greater than the 0 used by most modules,
     * including md_pm.c.  This is to ensure that we can override some
     * of PM's global configuration, such as liveness grace period.
     *
     * We need modrf_namespace_unrestricted to allow us to set nodes
     * from our initial values function that are not underneath our
     * module root.
     */
    err = mdm_add_module(
                    "agentd_pm",
                    1,
                    "/pm/nokeena/agentd",
                    NULL,
                    modrf_namespace_unrestricted,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    200,
                    0,
                    md_generic_upgrade_downgrade,
                    &md_agentd_pm_ug_rules,
                    md_nkn_pm_add_initial,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &module);
    bail_error(err);

    /* Upgrade processing nodes */
    err = md_upgrade_rule_array_new(&md_agentd_pm_ug_rules);
    bail_error(err);
    ra = md_agentd_pm_ug_rules;

bail:
    return err;
}
