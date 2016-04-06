#ifndef QTKEYCHAIN_GNOME_P_H
#define QTKEYCHAIN_GNOME_P_H

#include <QLibrary>

class GnomeKeyring : private QLibrary {
public:
    enum Result {
        RESULT_OK,
        RESULT_DENIED,
        RESULT_NO_KEYRING_DAEMON,
        RESULT_ALREADY_UNLOCKED,
        RESULT_NO_SUCH_KEYRING,
        RESULT_BAD_ARGUMENTS,
        RESULT_IO_ERROR,
        RESULT_CANCELLED,
        RESULT_KEYRING_ALREADY_EXISTS,
        RESULT_NO_MATCH
    };

    enum ItemType {
        ITEM_GENERIC_SECRET = 0,
        ITEM_NETWORK_PASSWORD,
        ITEM_NOTE,
        ITEM_CHAINED_KEYRING_PASSWORD,
        ITEM_ENCRYPTION_KEY_PASSWORD,
        ITEM_PK_STORAGE = 0x100
    };

    enum AttributeType {
        ATTRIBUTE_TYPE_STRING,
        ATTRIBUTE_TYPE_UINT32
    };

    typedef char gchar;
    typedef void* gpointer;
    typedef bool gboolean;
    typedef struct {
        ItemType item_type;
        struct {
            const gchar* name;
            AttributeType type;
        } attributes[32];
    } PasswordSchema;

    typedef void ( *OperationGetStringCallback )( Result result, bool binary,
                                                  const char* string, gpointer data );
    typedef void ( *OperationDoneCallback )( Result result, gpointer data );
    typedef void ( *GDestroyNotify )( gpointer data );

    static const char* GNOME_KEYRING_DEFAULT;

    static bool isAvailable();

    static gpointer store_network_password( const gchar* keyring, const gchar* display_name,
                                            const gchar* user, const gchar* server,
                                            const gchar* type, const gchar* password,
                                            OperationDoneCallback callback, gpointer data, GDestroyNotify destroy_data );

    static gpointer find_network_password( const gchar* user, const gchar* server,
                                           const gchar* type,
                                           OperationGetStringCallback callback,
                                           gpointer data, GDestroyNotify destroy_data );

    static gpointer delete_network_password( const gchar* user, const gchar* server,
                                             OperationDoneCallback callback, gpointer data, GDestroyNotify destroy_data );
private:
    GnomeKeyring();

    static GnomeKeyring& instance();

    const PasswordSchema* NETWORK_PASSWORD;
    typedef gboolean ( is_available_fn )( void );
    typedef gpointer ( store_password_fn )( const PasswordSchema* schema, const gchar* keyring,
                                            const gchar* display_name, const gchar* password,
                                            OperationDoneCallback callback, gpointer data, GDestroyNotify destroy_data,
                                            ... );
    typedef gpointer ( find_password_fn )( const PasswordSchema* schema,
                                           OperationGetStringCallback callback, gpointer data, GDestroyNotify destroy_data,
                                           ... );
    typedef gpointer ( delete_password_fn )( const PasswordSchema* schema,
                                             OperationDoneCallback callback, gpointer data, GDestroyNotify destroy_data,
                                             ... );

    is_available_fn* is_available;
    find_password_fn* find_password;
    store_password_fn* store_password;
    delete_password_fn* delete_password;
};


#endif
