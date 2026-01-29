#ifndef packettypes_h
#define packettypes_h

typedef enum {

    // Login and Registration Packets
    LOGIN = 1, // Authentication packet
    REGISTER, // Registration packet
    EXIT, // Exit packet

    // Message Requests (I'm going to start sending you a message) Sender -> Server
    MSG_ONE_REQUEST, // One-to-one message packet
    MSG_BROADCAST_REQUEST, // Broadcast message packet
    MSG_GROUP_REQUEST, // Group message packet

    // Server -> Destination Client
    MSG_ONE_WARNING, // One-to-one message packet
    MSG_BROADCAST_WARNING, // Broadcast message packet
    MSG_GROUP_WARNING, // Group message packet

    // Message Char Types:  Sender -> Server -> Destination Client(s)
    MSG_ONE, // One-to-one message packet
    MSG_BROADCAST, // Broadcast message packet
    MSG_GROUP, // Group message packet

    // Message Requests Responses
    MSG_OK, // You can start sending messages packet
    MSG_NOK, // You cannot start sending messages packet

    MSG_END, // End of message packet

    // Message Acknowledgments
    MSG_NACK, // Negative acknowledgment packet

    //Groups Management
    GROUP_CREATE, // Create group packet Group creator -> Server
    GROUP_INVITE, // Invite to group packet Group creator -> Server
    GROUP_KICK, // Kick from group packet
    GROUP_LEAVE, // Leave group packet
    GROUP_DELETE, // Delete group packet
    
    GROUP_INFO, // Group information packet
    GROUP_OK, // Group Management success packet
    GROUP_NOK, // Group Management failure packet

    //File transfer
    FILE_SEND_REQUEST, // Request to send a file packet
    FILE_SEND_WARNING, // File packet being sent
    FILE_SEND_ACCEPT, // Accept to receive a file packet
    FILE_SEND_DECLINE, // Decline to receive a file packet

    HEARTBEAT, // Heartbeat packet to check if the client is still active

    SERVER_ANNOUNCEMENT, // Server announcement packet

    LOGIN_OK, // Login successful packet
    LOGIN_NOK, // Login failed packet

    LOGOUT_OK, // Logout successful packet
    LOGOUT_NOK, // Logout failed packet

} PacketType;

#endif // packettypes_h