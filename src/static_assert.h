/*
 * static_assert.h
 *
 * Created: 4/20/2014 5:07:44 PM
 *  Author: clarkson
 */

#ifndef STATIC_ASSERT_H_
#define STATIC_ASSERT_H_

/*
 * Compile Time Assertion:
 *  Usage: STATIC_ASSERT( (msg_count > last_msg), To_Many_Messages_Defined_for_array_size );
 *
 *  STATIC_ASSERT( 1 == 2, One_Not_Equal_To_Two ); that looks like:
 *   assertion_failed_at_line_767_One_Not_Equal_To_Two
 */

#define STATIC_ASSERT_NAME_(line, message) STATIC_ASSERT_NAME2_(line, message)
#define STATIC_ASSERT_NAME2_(line, message) assertion_failed_at_line_##line##_##message
#define STATIC_ASSERT(claim, message)                              \
  typedef struct                                                   \
  {                                                                \
    char STATIC_ASSERT_NAME_(__LINE__, message)[(claim) ? 1 : -1]; \
  } STATIC_ASSERT_NAME_(__LINE__, message)

#endif /* STATIC_ASSERT_H_ */