
0                     Reserved
1                     Page code (0xC0)
2                     Reserved
3                     Page data length (24)
4                     Page version: 03
5                     Bit 0 = Is_TPVV flag (1 for TPVV, otherwise 0)
                      Bit 1 = Is_Snapshot (1 for snapshot, otherwise 0)
                      Bit 2 = TPVV_Reclaim_Supported (1 if supported, otherwise 0)
                      Bit 3 = TPVV_SizesValid (1 if bytes 6-25 contain valid TPVV size information)
                      Bit 4 = ATS_Supported (1 if supported, otherwise 0)
                      Bit 5 = XCOPY_Supported (1 if supported, otherwise 0)
                      Bit 6 = VV Info Valid (1 if VV and domain info are valid)
                      Bit 7 = Reserved
6-7                   Reserved
8-11                  Size of TPVV allocation unit
12-19                 Size of data pool allocated to this TPVV
20-27                 Amount of space actually allocated to this TPVV
28-35                 VV ID
36-39                 Domain ID
40-43                 VV name length (fixed at 32 in this version)
44-75                 VV name
76-79                 Domain name length (fixed at 32 in this version)
80-111                Domain name
112-115               User CPG name length (fixed at 32 in this version)
116-147               User CPG name
148-151               Snap CPG name length (fixed at 32 in this version)
152-183               Snap CPG name
184-187               VV Policy
                      Bit 0 = Stale_SS (1 if invalid snapshots are permitted)
                      Bit 1 = One_Host (1 if volume can be exported only to a single host)
                      Bit 2 = TP_Bzero (1 if remainder of partially-written pages is zeroed)
                      Bit 3 = Zero_Detect (1 if system will scan for zeros in incoming write data)
                      Bit 4-31 = Reserved

