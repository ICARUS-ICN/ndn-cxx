/**
 * @author: Jeff Thompson
 * See COPYING for copyright and distribution information.
 */

#ifndef NDN_NODE_HPP
#define NDN_NODE_HPP

#include "common.hpp"
#include "interest.hpp"
#include "transport/udp-transport.hpp"
#include "encoding/binary-xml-element-reader.hpp"

namespace ndn {

/**
 * An OnData function object is used to pass a callback to expressInterest.
 */
typedef func_lib::function<void(const ptr_lib::shared_ptr<const Interest> &, const ptr_lib::shared_ptr<Data> &)> OnData;

/**
 * An OnTimeout function object is used to pass a callback to expressInterest.
 */
typedef func_lib::function<void(const ptr_lib::shared_ptr<const Interest> &)> OnTimeout;

class Face;
  
class Node : public ElementListener {
public:
  /**
   * Create a new Node for communication with an NDN hub with the given Transport object and connectionInfo.
   * @param transport A shared_ptr to a Transport object used for communication.
   * @param transport A shared_ptr to a Transport::ConnectionInfo to be used to connect to the transport.
   */
  Node(const ptr_lib::shared_ptr<Transport> &transport, const ptr_lib::shared_ptr<const Transport::ConnectionInfo> &connectionInfo)
  : transport_(transport), connectionInfo_(connectionInfo)
  {
  }
  
  /**
   * Create a new Node for communication with an NDN hub at host:port using the default UdpTransport.
   * @param host The host of the NDN hub.
   * @param port The port of the NDN hub.
   */
  Node(const char *host, unsigned short port)
  : transport_(new UdpTransport()), connectionInfo_(new UdpTransport::ConnectionInfo(host, port))
  {
  }
  
  /**
   * Create a new Node for communication with an NDN hub at host with the default port 9695 and using the default UdpTransport.
   * @param host The host of the NDN hub.
   */
  Node(const char *host)
  : transport_(new UdpTransport()), connectionInfo_(new UdpTransport::ConnectionInfo(host, 9695))
  {
  }

  /**
   * Send the Interest through the transport, read the entire response and call onData(interest, data).
   * @param interest A reference to the Interest.  This copies the Interest.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   * @param onTimeout A function object to call if the interest times out.  If onTimeout is an empty OnTimeout(), this does not use it.
   * This copies the function object, so you may need to use func_lib::ref() as appropriate.
   */
  void expressInterest(const Interest &interest, const OnData &onData, const OnTimeout &onTimeout);

  /**
   * Send the Interest through the transport, read the entire response and call onData(interest, data).
   * @param interest A reference to the Interest.  This copies the Interest.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   */
  void expressInterest(const Interest &interest, const OnData &onData) {
    expressInterest(interest, onData, OnTimeout());
  }

  /**
   * Encode name as an Interest. If interestTemplate is not 0, use its interest selectors.
   * Send the interest through the transport, read the entire response and call onData(interest, data).
   * @param name A reference to a Name for the interest.  This copies the Name.
   * @param interestTemplate if not 0, copy interest selectors from the template.   This does not keep a pointer to the Interest object.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   * @param onTimeout A function object to call if the interest times out.  If onTimeout is an empty OnTimeout(), this does not use it.
   * This copies the function object, so you may need to use func_lib::ref() as appropriate.
   */
  void expressInterest(const Name &name, const Interest *interestTemplate, const OnData &onData, const OnTimeout &onTimeout);

  /**
   * Encode name as an Interest, using a default interest lifetime.
   * Send the interest through the transport, read the entire response and call onData(interest, data).
   * @param name A reference to a Name for the interest.  This copies the Name.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   * @param onTimeout A function object to call if the interest times out.  If onTimeout is an empty OnTimeout(), this does not use it.
   * This copies the function object, so you may need to use func_lib::ref() as appropriate.
   */
  void expressInterest(const Name &name, const OnData &onData, const OnTimeout &onTimeout) 
  {
    expressInterest(name, 0, onData, onTimeout);
  }
  
  /**
   * Encode name as an Interest. If interestTemplate is not 0, use its interest selectors.
   * Send the interest through the transport, read the entire response and call onData(interest, data).
   * @param name A reference to a Name for the interest.  This copies the Name.
   * @param interestTemplate if not 0, copy interest selectors from the template.   This does not keep a pointer to the Interest object.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   */
  void expressInterest(const Name &name, const Interest *interestTemplate, const OnData &onData)
  {
    expressInterest(name, interestTemplate, onData, OnTimeout());
  }
  
  /**
   * Encode name as an Interest, using a default interest lifetime.
   * Send the interest through the transport, read the entire response and call onData(interest, data).
   * @param name A reference to a Name for the interest.  This copies the Name.
   * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
   * use func_lib::ref() as appropriate.
   */
  void expressInterest(const Name &name, const OnData &onData)
  {
    expressInterest(name, 0, onData);
  }

  /**
   * Process any data to receive.  For each element received, call onReceivedElement.
   * This is non-blocking and will return immediately if there is no data to receive.
   * You should repeatedly call this from an event loop, with calls to sleep as needed so that the loop doesn't use 100% of the CPU.
   * @throw This may throw an exception for reading data or in the callback for processing the data.  If you
   * call this from an main event loop, you may want to catch and log/disregard all exceptions.
   */
  void processEvents();
  
  const ptr_lib::shared_ptr<Transport> &getTransport() { return transport_; }
  
  const ptr_lib::shared_ptr<const Transport::ConnectionInfo> &getConnectionInfo() { return connectionInfo_; }

  void onReceivedElement(const unsigned char *element, unsigned int elementLength);
  
  void shutdown();

private:
  class PitEntry {
  public:
    /**
     * Create a new PitEntry and set the timeoutTime_ based on the current time and the interest lifetime.
     * @param interest A shared_ptr for the interest.
     * @param onData A function object to call when a matching data packet is received.  This copies the function object, so you may need to
     * use func_lib::ref() as appropriate.
     * @param onTimeout A function object to call if the interest times out.  If onTimeout is an empty OnTimeout(), this does not use it.
     * This copies the function object, so you may need to use func_lib::ref() as appropriate.
     */
    PitEntry(const ptr_lib::shared_ptr<const Interest> &interest, const OnData &onData, const OnTimeout &onTimeout);
    
    const ptr_lib::shared_ptr<const Interest> &getInterest() { return interest_; }
    
    const OnData &getOnData() { return onData_; }
    
    /**
     * Get the struct ndn_Interest for the interest_.
     * WARNING: Assume that this PitEntry was created with new, so that no copy constructor is invoked between calls.
     * This class is private to Node and only used by its methods, so this should be OK.
     * TODO: Doesn't this functionality belong in the Interest class?
     * @return A reference to the ndn_Interest struct.
     * WARNING: The resulting pointers in are invalid uses getInterest() to manipulate the object which could reallocate memory.
     */
    const struct ndn_Interest &getInterestStruct()
    {
      return interestStruct_;
    }
    
    /**
     * If this interest is timed out, call onTimeout_ (if defined) and return true.
     * @param parent The parent Node for the UpcallInfo.
     * @param nowMilliseconds The current time in milliseconds from gettimeofday.
     * @return true if this interest timed out and the timeout callback was called, otherwise false.
     */
    bool checkTimeout(Node *parent, double nowMilliseconds);
    
  private:
    ptr_lib::shared_ptr<const Interest> interest_;
    std::vector<struct ndn_NameComponent> nameComponents_;
    std::vector<struct ndn_ExcludeEntry> excludeEntries_;
    struct ndn_Interest interestStruct_;
  
    const OnData onData_;
    const OnTimeout onTimeout_;
    double timeoutTimeMilliseconds_; /**< The time when the interest times out in milliseconds according to gettimeofday, or -1 for no timeout. */
  };
  
  /**
   * Find the entry from the pit_ where the name conforms to the entry's interest selectors, and
   * the entry interest name is the longest that matches name.
   * @param name The name to find the interest for (from the incoming data packet).
   * @return The index in pit_ of the pit entry, or -1 if not found.
   */
  int getEntryIndexForExpressedInterest(const Name &name);
  
  ptr_lib::shared_ptr<Transport> transport_;
  ptr_lib::shared_ptr<const Transport::ConnectionInfo> connectionInfo_;
  std::vector<ptr_lib::shared_ptr<PitEntry> > pit_;
};

}

#endif