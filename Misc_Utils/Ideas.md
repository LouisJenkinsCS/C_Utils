MU_Logger.h:
    * Create a simple formatter to determine logger format instead of hard coding it!.
        - Create a simple MU_Logger_Format_t type.
            + Will contain a char *format string, probably for each logger level.
        - Format based on % prefix.
            + %fnc = Function
            + %lno = Line Number
            + %lvl = Log Level
            + %fle = File
            + %tDn = Day as number (I.E 5 for 5th day of month.)
            + %tDS = Day as long string. (I.E Wednesday)
            + %tDs = Day as short string (I.E Wed)
            + %tMn = Month as number (I.E 3 for 3rd month of year.)
            + %tMS = Month as long string (I.E January)
            + %tMs = Month as short string (I.E Jan)
            + %tPr = AM / PM
            + %tYr = Year
            + %tMn = Minutes
            + %tHr = Hours
            + %tSc = Seconds
            + %msg = Message
        - Example format
            + "[%tMs %tDn, %tYr %tHr:%tMn:%tSc %tPr](%Fnc:%lno) %lvl: %fnc()->\"%msg\""
            + [Jul 22, 2015 5:19:45 PM](./SU_Regex.c:178) ERROR: SU_Regex_parse()->"Was unable to successfully parse string!"
    * Add configuration file support
        - Take an optional argument as to whether or not to read settings from a configuration file. Maybe as another constructor
            + MU_Logger_create_from_file(MU_Logger_t *logger, FILE *file);
    * Add syslog support
        - 