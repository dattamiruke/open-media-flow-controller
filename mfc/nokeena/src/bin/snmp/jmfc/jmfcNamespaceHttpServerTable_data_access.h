/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 12854 $ of $
 *
 * $Id:$
 */
#ifndef JMFCNAMESPACEHTTPSERVERTABLE_DATA_ACCESS_H
#define JMFCNAMESPACEHTTPSERVERTABLE_DATA_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif


    /*
     *********************************************************************
 * function declarations
 */

    /*
     *********************************************************************
 * Table declarations
 */
/**********************************************************************
 **********************************************************************
 ***
 *** Table jmfcNamespaceHttpServerTable
 ***
 **********************************************************************
 **********************************************************************/
/*
 * JUNIPER-MFC-MIB::jmfcNamespaceHttpServerTable is subid 3 of jmfcNamespace.
 * Its status is Current.
 * OID: .1.3.6.1.4.1.2636.1.2.1.4.3, length: 12
*/


    int            
        jmfcNamespaceHttpServerTable_init_data
        (jmfcNamespaceHttpServerTable_registration *
         jmfcNamespaceHttpServerTable_reg);


    /*
     * TODO:180:o: Review jmfcNamespaceHttpServerTable cache timeout.
     * The number of seconds before the cache times out
     */
#define JMFCNAMESPACEHTTPSERVERTABLE_CACHE_TIMEOUT   15

    void           
        jmfcNamespaceHttpServerTable_container_init(netsnmp_container **
                                                    container_ptr_ptr,
                             netsnmp_cache *cache);
    void           
        jmfcNamespaceHttpServerTable_container_shutdown(netsnmp_container *
                                                        container_ptr);

    int            
        jmfcNamespaceHttpServerTable_container_load(netsnmp_container *
                                                    container);
    void           
        jmfcNamespaceHttpServerTable_container_free(netsnmp_container *
                                                    container);

    int            
        jmfcNamespaceHttpServerTable_cache_load(netsnmp_container *
                                                container);
    void           
        jmfcNamespaceHttpServerTable_cache_free(netsnmp_container *
                                                container);

    /*
    ***************************************************
    ***             START EXAMPLE CODE              ***
    ***---------------------------------------------***/
    /*
     *********************************************************************
 * Since we have no idea how you really access your data, we'll go with
 * a worst case example: a flat text file.
 */
#define MAX_LINE_SIZE 256
    /*
    ***---------------------------------------------***
    ***              END  EXAMPLE CODE              ***
    ***************************************************/
    int            
        jmfcNamespaceHttpServerTable_row_prep
        (jmfcNamespaceHttpServerTable_rowreq_ctx * rowreq_ctx);



#ifdef __cplusplus
}
#endif
#endif /* JMFCNAMESPACEHTTPSERVERTABLE_DATA_ACCESS_H */
