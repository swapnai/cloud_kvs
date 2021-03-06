#include "MP1Node.h"
#include "gtest/gtest.h"

namespace {

  class MP1NodeTest : public ::testing::Test {
  protected:
    // Init work here
    MP1NodeTest() {
    }
    // no exception throwing code in dtor
    virtual ~MP1NodeTest() {
    }
    // after ctor
    virtual void SetUp() {
      // add tests for non-static methods - TBD
      /*      MP1Node m1, m2, m3;
	      par = Params();
	      
	      srand (time(NULL));
	      par.setparams();
	      log = Log(par);
	      en = EmulNet(par);
	      Member mem1();
      */
      
      
    }
    // right before dtor
    virtual void TearDown() {
    }
    // Class objects here
  };

  TEST(SerializeTest, EmptyML) {

    vector<MemberListEntry> empty;
    char* msg;
    Address addr("1:0:1:1");
    
    size_t sz = MP1Node::serialize(empty, msg, &addr);
    EXPECT_EQ(sz, sizeof(MessageHdr) + sizeof(Address) + sizeof(int));

  }

}// namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
