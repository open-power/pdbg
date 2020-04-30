#ifndef __LIBPDBG_H
#define __LIBPDBG_H

/**
 * @file libpdbg.h
 * @author Alistair Popple
 * @brief libpdbg targeting API defintions for use by external programs
 *
 * libpdbg defines a targeting framework based on a device-tree to
 * describe the topology of the system being modelled. This file
 * describes all the interfaces available for external programs to
 * discover and interact with the system topology.
 *
 * It also provides API functions for backend hardware access
 * (eg. SCOM acess).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct pdbg_target
 * @brief PowerPC FSI Debugger target
 *
 * An opaque type handle representing a target in the
 * topology. External programs will generally use a pointer to a
 * pdbg_target to interact with the API.
 */
struct pdbg_target;

/**
 * @struct pdbg_target_class
 * @brief PowerPC FSI Debugger target class
 *
 * An opaque type representing a particular type of pdbg_target. For
 * example there may be multiple targets representing an xbus in the
 * hierarchy and they will all be part of the same pdbg_target_class.
 */
struct pdbg_target_class;

/**
 * @brief Find the next compatible target
 * @param[in] root the head pdbg_target
 * @param[in] prev the last pdbg_target in the class
 * @param[in] compat compatible string to match
 * @return the next compatible pdbg_target*
 *
 * This function is not intended for direct use by an
 * application. Instead it is used as an iterator by some of the
 * for_each_* macros.
 */
struct pdbg_target *__pdbg_next_compatible_node(struct pdbg_target *root,
                                                struct pdbg_target *prev,
                                                const char *compat);

/**
 * @brief Find the next pdbg_target* given a class
 * @param[in] klass the klass of interest
 * @param[in] parent the head pdbg_target
 * @param[in] last the last pdbg_target in the klass
 * @param[in] system only search real system targets
 * @return the next pdbg_target* iff successful, otherwise NULL
 *
 * This function is not intended for direct use by an
 * application. Instead it is used as an iterator by some of the
 * for_each_* macros.
 */
struct pdbg_target *__pdbg_next_target(const char *klass, struct pdbg_target *parent, struct pdbg_target *last, bool system);

/**
 * @brief Find the next child pdbg_target* given a parent target
 * @param[in] parent the head pdbg_target
 * @param[in] last the last pdbg_target in the class
 * @param[in] system only search real system targets
 * @return the next pdbg_target* iff successful, otherwise NULL
 *
 * This function is not intended for direct use by an
 * application. Instead it is used as an iterator by some of the
 * for_each_* macros.
 */
struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last, bool system);

/**
 * @brief Describes the current status of a particular target
 * instance.
 *
 * Each target has an status associated with it read from the device
 * tree.  If no status property is present in the device tree it
 * defaults to PDBG_TARGET_UNKNOWN. The status of a target affects how
 * it and it's children/parent targets are probed.
 *
 * Application code may set some targets to specific status. For
 * example an application may use pdbg_target_status_set() to mark a
 * pdbg_target disabled to avoid probing it. Some status state may
 * only be set by core library code.
 */
enum pdbg_target_status {
	/**
	 * The target has not been probed but will be probed if
	 * required.
	 */
	PDBG_TARGET_UNKNOWN = 0,

	/**
	 * The target exists and has been probed, will be released by
	 * the release call.
	 */
	PDBG_TARGET_ENABLED,

	/**
	 * The target has not been probed and will never be
	 * probed. Application code may use this to prevent
	 * probing of certain targets if it knows they are
	 * unnecessary.
	 */
	PDBG_TARGET_DISABLED,

	/**
	 * The target has not been probed but an error will be
	 * reported if it does not exist as it is required for correct
	 * operation. Application code may set this.
	 */
	PDBG_TARGET_MUSTEXIST,

	/**
	 * The target has been probed but did not exist. It will never be
	 * reprobed.
	 */
	PDBG_TARGET_NONEXISTENT,

	/**
	 * The target was enabled and has now been released.
	 */
	PDBG_TARGET_RELEASED,
};

/**
 * @brief Describes the various methods (referred to as backends) for
 * accessing hardware depending on where the code is executed.
 *
 * Some backends require extra parameters to correctly probe the hardware.
 *
 * If no backend is set, default backend will be determined depending on where
 * the code is executed.
 */
enum pdbg_backend {
	/**
	 * The default backend, this will cause the backend to be determined
	 * automatically.
	 */
	PDBG_DEFAULT_BACKEND = 0,

	/**
	 * The bit-banging backend using GPIO to directly access FSI bus.
	 * This is experimental code and should not be used in production.
	 */
	PDBG_BACKEND_FSI,

	/**
	 * This is P8 only backend using i2c bus to probe the hardware.
	 *
	 * This backend requires the extra information about the path of
	 * the i2c device to access the bus.
	 */
	PDBG_BACKEND_I2C,

	/**
	 * This is the default backend for BMC.  It uses BMC kernel drivers to
	 * access the hardware.
	 */
	PDBG_BACKEND_KERNEL,

	/**
	 * This is a test backend.  It is used for testing code.
	 */
	PDBG_BACKEND_FAKE,

	/**
	 * This is the default backend for host.  It uses host kernel/skiboot
	 * drivers to access the hardware.
	 */
	PDBG_BACKEND_HOST,

	/**
	 * This backend uses croserver to access hardware.
	 *
	 * This backend requires extra information about the system type and
	 * the BMC network address / hostname.  For example p9@spoon2-bmc.
	 */
	PDBG_BACKEND_CRONUS,

	/**
	 * This backend uses sbefifo kernel driver on BMC to access hardware
	 * via SBE.
	 */
	PDBG_BACKEND_SBEFIFO,
};

/**
 * @brief Loop over compatible targets
 * @param[in] parent if non-NULL only iterate over children of the given target
 * @param[in] target the current pdbg_target
 * @param[out] compat comptabile key to match against
 *
 * The compatible properties are used internally and their use should
 * be mostly avoided by application code.
 */
#define pdbg_for_each_compatible(parent, target, compat)		\
        for (target = NULL;                                             \
             (target = __pdbg_next_compatible_node(parent, target, compat)) != NULL;)

/**
 * @brief Loop accross targets of the given class/type
 * @param[in] class the class/type of target required
 * @param[in] parent if non-NULL only iterate over children of the given target
 * @param[in] target the current pdbg_target
 *
 * Each target gets assigned a specific class/type as specified in the
 * device tree. This macro can be used to iterate over every
 * pdbg_target of the given pdbg_target_class.
 *
 * If parent is non-NULL only targets with the given parent will be
 * iterated over. If parent is null all targets in the given
 * class/type will be returned.
 */
#define pdbg_for_each_target(class, parent, target)			\
	for (target = __pdbg_next_target(class, parent, NULL, true);	\
	     target;							\
	     target = __pdbg_next_target(class, parent, target, true))

/**
 * @brief Loop over targets of the given class/type
 * @param[in] class the class/type of target required
 * @param[in] target the current pdbg_target
 *
 * Convenience function equivalent to calling pdbg_for_each_target()
 * with parent == NULL.
 */
#define pdbg_for_each_class_target(class, target)			\
	for (target = __pdbg_next_target(class, NULL, NULL, true);	\
	     target;						\
	     target = __pdbg_next_target(class, NULL, target, true))

/**
 * @brief Loop over all direct children of the given target
 * @param[in] parent the head pdbg_target
 * @param[in] target the current pdbg_target
 *
 * Loops over all direct children of the parent pdbg_target. Does not
 * descend (ie. recursively) loop over children.
 */
#define pdbg_for_each_child_target(parent, target)	      \
	for (target = __pdbg_next_child_target(parent, NULL, true); \
	     target;					      \
	     target = __pdbg_next_child_target(parent, target, true))

/**
 * @brief Find a target parent of the given class/type
 * @param[in] klass the desired parent class
 * @param[in] target the pdbg_target
 * @return struct pdbg_target* the parent target of the given
 * class. If the target does not have a parent of the given class
 * returns NULL
 */
struct pdbg_target *pdbg_target_parent(const char *klass, struct pdbg_target *target);

/**
 * @brief Find a virtual target linked to target parent of the given class/type
 * @param[in] klass the desired parent class
 * @param[in] target the pdbg_target
 * @return struct pdbg_target* the parent target of the given
 * class. If the target does not have a parent of the given class
 * returns NULL
 */
struct pdbg_target *pdbg_target_parent_virtual(const char *klass, struct pdbg_target *target);

/**
 * @brief Find a target parent of the given class
 * @param[in] klass the given class
 * @param[in] target the pdbg_target
 * @return struct pdbg_target* the first parent target of the given class
 *
 * @note Same as pdbg_target_parent() but instead of returning NULL
 * causes an assert failure when a parent target of the given
 * class/type doesn't exist.
 */
struct pdbg_target *pdbg_target_require_parent(const char *klass, struct pdbg_target *target);

/**
 * @brief Overwrite the given property in device tree
 * @param[in] target pdbg_target to set the property on
 * @param[in] name name of the property to set
 * @param[in] val value of the property to set
 * @param[in] size size of the value in bytes
 * @return true on success, false on failure
 *
 * This function will only modify the existing property in the device tree,
 * provided the value is of the same length as the previous value stored in
 * the device tree.
 */
bool pdbg_target_set_property(struct pdbg_target *target, const char *name, const void *val, size_t size);

/**
 * @brief Get the given property and return the size
 * @param[in] target pdbg_target to get the property from
 * @param[in] name name of the property to get
 * @param[out] size size of the returned property value in bytes
 * @return void* a pointer to the property value or NULL if not found
 */
const void *pdbg_target_property(struct pdbg_target *target, const char *name, size_t *size);

/**
 * @brief Overwrite the value of given attribute in device tree
 *
 * The attributes are treated as 1, 2, 4, or 8 byte integer arrays.
 *
 * @param[in] target pdbg_target to set the attribute on
 * @param[in] name name of the attribute to set
 * @param[in] size Size of element
 * @param[in] count Number of elements in the buffer
 * @param[in] val value of the attribute to set
 * @return true on success, false on failure
 *
 * This function will update the attribute value provided the count of
 * elements and the size of element matches.
 */
bool pdbg_target_set_attribute(struct pdbg_target *target, const char *name, uint32_t size, uint32_t count, const void *val);

/**
 * @brief Get the value of the given attribute from device tree
 *
 * The attributes are treated as 1, 2, 4, or 8 byte integer arrays.
 *
 * @param[in] target pdbg_target to get the attribute from
 * @param[in] name name of the attribute to get
 * @param[in] size Size of element
 * @param[in] count Number of elements in the buffer
 * @param[out] val value of the attribute
 * @return true on success, false on failure
 *
 * This function will copy the attribute value in the given buffer, provided
 * the count of elements and the size of element matches.
 */
bool pdbg_target_get_attribute(struct pdbg_target *target, const char *name, uint32_t size, uint32_t count, void *val);

/**
 * @brief Overwrite the value of given attribute in device tree
 *
 * The attribute value is treated as a packed stream of 1, 2, 4, or 8 byte
 * integers.  The specification describes how the integers are packed.
 *
 * Example:
 *    A stream of uint32_t, uint8_t, uint16_t, uint32_t is specified as
 *    "4124".
 *
 * @param[in] target pdbg_target to set the attribute on
 * @param[in] name name of the attribute to set
 * @param[in] spec specification of packed integers
 * @param[in] val value of the attribute to set
 * @return true on success, false on failure
 *
 * This function will update the attribute value provided the size matches.
 */
bool pdbg_target_set_attribute_packed(struct pdbg_target *target, const char *name, const char *spec, const void *val);

/**
 * @brief Get the value of the given attribute from device tree
 *
 * The attribute value is treated as a packed stream of 1, 2, 4, or 8 byte
 * integers.  The specification describes how the integers are packed.
 *
 * @see pdbg_target_set_attribute_packed
 *
 * @param[in] target pdbg_target to get the attribute from
 * @param[in] name name of the attribute to get
 * @param[in] spec specification of packed integers
 * @param[out] val value of the attribute
 * @return true on success, false on failure
 *
 * This function will copy the attribute value in the given buffer, provided
 * the specification matches.
 */
bool pdbg_target_get_attribute_packed(struct pdbg_target *target, const char *name, const char *spec, void *val);

/**
 * @brief Get the given property value as a uint32_t
 * @param[in] target pdbg_target to get the property from
 * @param[in] name name of the property to get
 * @param[out] val the property value cast a uint32_t
 * @return 0 iff successful, -1 otherwise
 *
 * Device tree properties are stored in as a series of bytes. This
 * function checks the property size and converts the property value
 * into a uint32 in the correct endian.
 *
 * @note If the property has an incorrect size it will raise an assert
 * error.
 */
int pdbg_target_u32_property(struct pdbg_target *target, const char *name, uint32_t *val);

/**
 * @brief Treat the property value as an array of uint32_t and return
 * the value of the given index
 * @param[in] target pdbg_target to get the property from
 * @param[in] name name of the property to get
 * @param[in] index array index of the u32 to return
 * @param[out] val pointer to return value, modified only if no error.
 * @return int 0 iff successful, otherwise -1
 *
 * Device tree properties are stored in as a series of bytes. This
 * function treats the byte stream as an array of uint32's and returns
 * the uint32 at the given array index, ensuring it is in the correct
 * endian.
 *
 * @note pdbg_target_u32_index() cannot check the property size is large
 * enough to contain the total array, however it will check that it is
 * large enough to contain the current index and raise an assert error
 * if it isn't.
 */
int pdbg_target_u32_index(struct pdbg_target *target, const char *name, int index, uint32_t *val);

/**
 * @brief Get the address offset for a pdbg_target
 * @param[in] target the pdbg_target
 * @param[out] size size of this targets address region
 * @return uint64_t address offset of the given pdbg_target
 *
 * Device tree specifies a method of specifing the address range of a
 * given node/target using the reg device tree property. This
 * specifies both the starting address and the size for a particular
 * region. This function returns both the starting address from the
 * reg property and the size.
 *
 * @note This function is only intended for use parsing the
 * device-tree standard reg property. Translation to an absolute
 * address within a given address space is handled by
 * pdbg_address_absolute().
 */
uint64_t pdbg_target_address(struct pdbg_target *target, uint64_t *size);

/**
 * @brief DEPRECATED, do not use
 *
 * These will be removed at some point and only exist for backwards
 * compatibility with older projects.
 */
#define pdbg_set_target_property(target, name, val, size)	\
	pdbg_target_set_property(target, name, val, size)

/**
 * @brief DEPRECATED, do not use
 *
 * These will be removed at some point and only exist for backwards
 * compatibility with older projects.
 */
#define pdbg_get_target_property(target, name, size)	\
	pdbg_target_property(target, name, size)

/**
 * @brief DEPRECATED, do not use
 *
 * These will be removed at some point and only exist for backwards
 * compatibility with older projects.
 */
#define pdbg_get_address(target, index, size)				\
	(index == 0 ? pdbg_target_address(target, size) : assert(0))

/**
 * @brief Set the hardware access method
 *
 * @param[in]  backend the pdbg_backend backend
 * @param[in]  backend_option extra information required by backend
 *
 * @return true on success, false if the backend information could not be
 * parsed.
 *
 * Must be called before calling pdbg_targets_init().
 */
bool pdbg_set_backend(enum pdbg_backend backend, const char *backend_option);

/**
 * @brief Initialises the targeting system from the given flattened device tree.
 *
 * @param [in]  fdt The system device tree pointer, NULL to use default
 * @return true on success, false on failure
 *
 * Must be called prior to using any other libpdbg functions.
 *
 * Device tree can also be specified using PDBG_DTB environment variable
 * pointing to system device tree.
 *
 * If the argument is NULL, then PDBG_DTB will override the default device
 * tree.  If the argument is not NULL, then that will override the default
 * device tree and device tree pointed by PDBG_DTB.
 *
 * @note This function can only be called once.
 */
bool pdbg_targets_init(void *fdt);

/**
 * @brief Probe all targets
 *
 * By default the pdbg_target_status gets initialised from the device
 * tree when calling pdbg_targets_init(). Most targets will not have a
 * specific status assigned to them in the device-tree, meaning they
 * will have a status of PDBG_TARGET_UNKNOWN.
 *
 * To determine a targets status it first must be probed. This
 * function probes all targets in the given device-tree, taking into
 * account any targets with a specific status set from the device-tree
 * or via pdbg_target_status_set().
 *
 * For example targets with a pdbg_target_status of
 * PDBG_TARGET_DISABLED will not be probed, and therefore any children
 * underneath it will also not be probed.
 */
void pdbg_target_probe_all(struct pdbg_target *parent);

/**
 * @brief Probe a specific target
 *
 * @param[in] target the specific pdbg_target to probe
 * @return pdbg_target_status after probing the given target
 *
 * If the target has already been probed (ie. it has a
 * pdbg_target_status() == PDBG_TARGET_ENABLED, PDBG_TARGET_DISABLED or
 * PDBG_TARGET_NONEXISTANT) simply returns the current status of the
 * target.
 *
 * If a target has not already been probed (ie. it has a
 * pdbg_target_status() == PDBG_TARGET_UNKNOWN or
 * PDBG_TARGET_MUSTEXIST) then probe the given target to determine its
 * state. This may require probing other targets as it will walk up
 * the tree looking for the first unprobed parent and then probe every
 * target in the tree between the given target and this parent to
 * determine the state of the given target.
 *
 * @note Currently trying to probe a target with a status of
 * PDBG_TARGET_RELEASED will result in an assert error. This may
 * change in future if there is a use case for re-probing targets that
 * have been released.
 */
enum pdbg_target_status pdbg_target_probe(struct pdbg_target *target);

/**
 * @brief Release the given target
 * @param[in] target the pdbg_target to release
 *
 * Releases the given target. Will first release any child targets if
 * neccessary. Once a target has been release it should not be used
 * again.
 *
 * @note It is currently not possible to reprobe a target once
 * released. This may change in future if a use case requiring this is
 * found.
 */
void pdbg_target_release(struct pdbg_target *target);

/**
 * @brief Get the current pdbg_target status
 * @param[in] target the pdbg_target
 * @return pdbg_target_status current pdbg_target status
 */
enum pdbg_target_status pdbg_target_status(struct pdbg_target *target);

/**
 * @brief Set the pdbg_target status
 * @param[in] target pdbg_target
 * @param[in] status one of PDBG_TARGET_DISABLED or PDBG_TARGET_MUSTEXIST
 * @return void
 *
 * @note The status may only be set to PDBG_TARGET_DISABLED or
 * PDBG_TARGET_MUSTEXIST as other states can only be determined by
 * probing. An assert error will be raised when attempting to set
 * other states.
 */
void pdbg_target_status_set(struct pdbg_target *target, enum pdbg_target_status status);

/**
 * @brief Searches up the tree and returns the first valid index found
 * @param[in] target the pdbg_target
 * @return uint32_t the target index or -1ULL if none was found
 */
uint32_t pdbg_target_index(struct pdbg_target *target);

/**
 * @brief Returns the full path to the given target
 * @param[in] target the pdbg_target
 * @return A null terminated string containing the path
 */
const char *pdbg_target_path(struct pdbg_target *target);

/**
 * @brief Return the pdbg_target with the given path
 * @param[in] target the pdbg_target for the root path, typically pdbg_target_root() or NULL for the system root
 * @param[in] path the path to the desired node
 * @return The pdbg_target at path or NULL if none found
 */
struct pdbg_target *pdbg_target_from_path(struct pdbg_target *target, const char *path);

/**
 * @brief Search up the tree for a parent target of the right class and returns its index
 * @param[in] target the pdbg_target
 * @param[in] klass the class of the parent
 * @return uint32_t  target parent  index, otherwise  -1 if  no parent
 * with an index could be found
 */
uint32_t pdbg_parent_index(struct pdbg_target *target, char *klass);

/**
 * @brief Get the target class name
 * @param[in] target the pdbg_target
 * @return char* the class name
 */
const char *pdbg_target_class_name(struct pdbg_target *target);

/**
 * @brief Get the target name
 * @param[in] target the pdbg_target
 * @return char* the target name
 */
const char *pdbg_target_name(struct pdbg_target *target);

/**
 * @brief Get the device node name
 * @param[in] target the pdbg_target
 * @return char* the dn name
 */
const char *pdbg_target_dn_name(struct pdbg_target *target);

/**
 * @brief Gets the priv member of pdbg_target
 * @param[in] target the pdbg_target
 * @return void* to priv
 *
 * The priv pointer may be used by applications to store any target
 * specific information related to a given target.
 */
void *pdbg_target_priv(struct pdbg_target *target);

/**
 * @brief Sets the priv member of pdbg_target
 * @param[in,out] target the pdbg_target
 * @param[in] priv pointer to application specific target data
 * @return void
 */
void pdbg_target_priv_set(struct pdbg_target *target, void *priv);

/**
 * @brief Gets the head pdbg_target (root node)
 * @return pdbg_target the root target
 */
struct pdbg_target *pdbg_target_root(void);

/**
 * @brief Check to see if a given compatible matches that of a pdbg_target
 * @param[in] target the pdbg_target
 * @param[in] compatible the compatible string we want to match against
 * @return bool true if pdbg_target is compatible
 */
bool pdbg_target_compatible(struct pdbg_target *target, const char *compatible);

const char *kernel_get_fsi_path(void);

/**
 * @brief Translate an address offset for a target to an absolute address
 * @param[in] target the current pdbg_target
 * @param[in, out] addr the target address offset. Returns the absolute address.
 * @return struct pdbg_target* the bus root target
 *
 * Translates the given target address into an absolute address on the
 * target bus such that a bus operation may be perfomed directly with
 * the given address offset on the returned bus root target.
 *
 * @note Currently this only supports pib/SCOM bus/targets. Work is
 * underway to extend this to all bus types.
 */
struct pdbg_target *pdbg_address_absolute(struct pdbg_target *target, uint64_t *addr);

/**
 * @brief Read a CFAM FSI register
 * @param[in] target the pdbg_target
 * @param[in] addr the CFAM address offset
 * @param[out] val the read data
 * @return int 0 if successful, -1 otherwise
 */
int fsi_read(struct pdbg_target *target, uint32_t addr, uint32_t *val);

/**
 * @brief Write a CFAM FSI register
 * @param[in] target the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[in] val the write data
 * @return int 0 if successful, -1 otherwise
 */
int fsi_write(struct pdbg_target *target, uint32_t addr, uint32_t val);

/**
 * @brief Write a CFAM FSI register with a mask
 * @param[in] target the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[in] val the write data
 * @param[in] mask The bits which are to be overwritten
 * @return int 0 if successful, -1 otherwise
 */
int fsi_write_mask(struct pdbg_target *target, uint32_t addr, uint32_t val, uint32_t mask);

/**
 * @brief Read a PIB SCOM register
 * @param[in] target the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[out] val the read data
 * @return int 0 if successful, -1 otherwise
 */
int pib_read(struct pdbg_target *target, uint64_t addr, uint64_t *val);

/**
 * @brief Write a PIB SCOM register
 * @param[in] target the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[out] val the write data
 * @return int 0 if successful, -1 otherwise
 */
int pib_write(struct pdbg_target *target, uint64_t addr, uint64_t val);

/**
 * @brief Write a PIB SCOM register with a mask
 * @param[in] target the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[in] val the write data
 * @param[in] mask The bits which are to be overwritten
 * @return int 0 if successful, -1 otherwise
 */
int pib_write_mask(struct pdbg_target *target, uint64_t addr, uint64_t val, uint64_t mask);

/**
 * @brief Wait for a SCOM register addr to match value & mask == data
 * @param[in] pib_dt the pdbg_target
 * @param[in] addr the address offset relative to target
 * @param[in] mask the data mask
 * @param[in] data value the masked register value must match
 * @return int 0 if successful, -1 otherwise
 */
int pib_wait(struct pdbg_target *pib_dt, uint64_t addr, uint64_t mask, uint64_t data);

/**
 * @struct thread_regs
 * @brief CPU per-thread registers
 *
 * See the POWER ISA documentation for a description of these
 * registers.
 */
struct thread_regs {
	uint64_t nia;
	uint64_t msr;
	uint64_t cfar;
	uint64_t lr;
	uint64_t ctr;
	uint64_t tar;
	uint32_t cr;
	uint64_t xer;
	uint64_t gprs[32];

	uint64_t lpcr;
	uint64_t ptcr;
	uint64_t lpidr;
	uint64_t pidr;
	uint64_t hfscr;
	uint32_t hdsisr;
	uint64_t hdar;
	uint64_t hsrr0;
	uint64_t hsrr1;
	uint64_t hdec;
	uint32_t heir;
	uint64_t hid;
	uint64_t hsprg0;
	uint64_t hsprg1;
	uint64_t fscr;
	uint32_t dsisr;
	uint64_t dar;
	uint64_t srr0;
	uint64_t srr1;
	uint64_t dec;
	uint64_t tb;
	uint64_t sprg0;
	uint64_t sprg1;
	uint64_t sprg2;
	uint64_t sprg3;
	uint64_t ppr;
};

/**
 * @brief Set the MSR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] val new value to set MSR to
 * @return 0 on success, -1 otherwise
 */
int thread_putmsr(struct pdbg_target *target, uint64_t val);

/**
 * @brief Read memory at an effective address on the given thread
 * @param[in] thread the thread target to operate on
 * @param[in] addr the effective address to read from
 * @param[out] value memory contents at the specified address
 */
int thread_getmem(struct pdbg_target *thread, uint64_t addr, uint64_t *value);

/**
 * @brief Set the next instruction address on a thread
 * @param[in] target the thread target to operate on
 * @param[in] val new value to set NIA to
 * @return 0 on success, -1 otherwise
 */
int thread_putnia(struct pdbg_target *target, uint64_t val);

/**
 * @brief Set a SPR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] spr the SPR number to write
 * @param[in] val new value to set the given SPR to
 * @return 0 on success, -1 otherwise
 */
int thread_putspr(struct pdbg_target *target, int spr, uint64_t val);

/**
 * @brief Set a GPR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] gpr the GPR number to write
 * @param[in] val new value to set the given GPR to
 * @return 0 on success, -1 otherwise
 */
int thread_putgpr(struct pdbg_target *target, int gpr, uint64_t val);

/**
 * @brief Get the MSR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] val value to of MSR on the given thread
 * @return 0 on success, -1 otherwise
 */
int thread_getmsr(struct pdbg_target *target, uint64_t *val);

/**
 * @brief Get the CR on a thread
 * @param[in] thread the thread target to operate on
 * @param[out] value current value of CR for the given thread
 * @return 0 on success, -1 otherwise
 */
int thread_getcr(struct pdbg_target *thread,  uint32_t *value);

/**
 * @brief Set the CR on a thread
 * @param[in] thread the thread target to operate on
 * @param[in] value new value to set CR to
 * @return 0 on success, -1 otherwise
 */
int thread_putcr(struct pdbg_target *thread,  uint32_t value);

/**
 * @brief Get the next instruction address on a thread
 * @param[in] target the thread target to operate on
 * @param[out] val current value of NIA on the thread
 * @return 0 on success, -1 otherwise
 */
int thread_getnia(struct pdbg_target *target, uint64_t *val);

/**
 * @brief Get a SPR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] spr the SPR number to read
 * @param[out] val value of the given SPR
 * @return 0 on success, -1 otherwise
 */
int thread_getspr(struct pdbg_target *target, int spr, uint64_t *val);

/**
 * @brief Get a GPR on a thread
 * @param[in] target the thread target to operate on
 * @param[in] gpr the GPR number to read
 * @param[out] val value of the given GPR
 * @return 0 on success, -1 otherwise
 */
int thread_getgpr(struct pdbg_target *target, int gpr, uint64_t *val);

/**
 * @brief Get the XER on a thread
 * @param[in] thread the thread target to operate on
 * @param[out] value current value of XER for the given thread
 * @return 0 on success, -1 otherwise
 */
int thread_getxer(struct pdbg_target *thread, uint64_t *value);

/**
 * @brief Set the XER on a thread
 * @param[in] thread the thread target to operate on
 * @param[in] value value to set XER to
 * @return 0 on success, -1 otherwise
 */
int thread_putxer(struct pdbg_target *thread, uint64_t value);

/**
 * @brief Get the value of all interesting registers on a thread
 * @param[in] target the thread target to operate on
 * @param[out] regs a pointer to a structure with register values
 * @return 0 on success, -1 otherwise
 */
int thread_getregs(struct pdbg_target *target, struct thread_regs *regs);

/**
 * @brief the pdbg thread states
 */
enum pdbg_sleep_state {
	PDBG_THREAD_STATE_RUN,   /**< RUN STATE   */
	PDBG_THREAD_STATE_DOZE,  /**< DOZE STATE  */
	PDBG_THREAD_STATE_NAP,   /**< NAP STATE   */
	PDBG_THREAD_STATE_SLEEP, /**< SLEEP STATE */
	PDBG_THREAD_STATE_STOP,  /**< STOP STATE  */
};

/**
 * @brief the pdbg_smt states
 */
enum pdbg_smt_state {
	PDBG_SMT_UNKNOWN, /**< UNKNOWN STATE */
	PDBG_SMT_1,       /**< SMT_1 STATE   */
	PDBG_SMT_2,       /**< SMT_2 STATE   */
	PDBG_SMT_4,       /**< SMT_4 STATE   */
	PDBG_SMT_8,       /**< SMT_8 STATE   */
};

/**
 * @struct thread_state
 * @brief Container of thread states
 */
struct thread_state {
	bool active;
	bool quiesced;
	enum pdbg_sleep_state sleep_state;
	enum pdbg_smt_state smt_state;
};

/**
 * @brief Start execution of the given thread
 * @param[in] target the thread target to operate on
 * @return 0 on success, -1 otherwise
 */
int thread_start(struct pdbg_target *target);

/**
 * @brief Step execution on the given thread
 * @param[in] target the thread target to operate on
 * @param[in] steps number of instructions to step
 * @return 0 on success, -1 otherwise
 */
int thread_step(struct pdbg_target *target, int steps);

/**
 * @brief Stop execution on the given thread
 * @param[in] target the thread target to operate on
 * @return 0 on success, -1 otherwise
 */
int thread_stop(struct pdbg_target *target);

/**
 * @brief sreset the thread
 * @param[in] target the thread target to operate on
 * @return 0 on success, -1 otherwise
 */
int thread_sreset(struct pdbg_target *target);

/**
 * @brief Start execution of all the threads
 * @return 0 on success, -1 otherwise
 */
int thread_start_all(void);

/**
 * @brief Step execution on all the threads
 * @return 0 on success, -1 otherwise
 */
int thread_step_all(void);

/**
 * @brief Stop execution on all the threads
 * @return 0 on success, -1 otherwise
 */
int thread_stop_all(void);

/**
 * @brief sreset all the threads
 * @return 0 on success, -1 otherwise
 */
int thread_sreset_all(void);

/**
 * @brief Return the thread status information
 *
 * @param[in]  target the thread target to operate on
 *
 * @return thread_state structure containing thread information
 */
struct thread_state thread_status(struct pdbg_target *target);

int getring(struct pdbg_target *chiplet_target, uint64_t ring_addr, uint64_t ring_len, uint32_t result[]);

/**
 * @brief Start HTM trace on a thread
 * @param[in] target thread to start tracing on
 * @returns 0 on success, -1 on failure
 */
int htm_start(struct pdbg_target *target);

/**
 * @brief Stop HTM trace of a thread
 * @param[in] target thread to start tracing on
 * @returns 0 on success, -1 on failure
 */
int htm_stop(struct pdbg_target *target);

/**
 * @brief Print current HTM trace status of a thread
 * @param[in] target thread to start tracing on
 * @returns 0 on success, -1 on failure
 */
int htm_status(struct pdbg_target *target);

/**
 * @brief Write HTM thread trace to a file
 * @param[in] target thread to start tracing on
 * @param[in] filename file to write the trace to
 * @returns 0 on success, -1 on failure
 */
int htm_dump(struct pdbg_target *target, char *filename);

/**
 * @brief Start recording HTM trace on a thread to a file
 * @param[in] target thread to start tracing on
 * @param[in] filename file to write the trace to
 * @returns 0 on success, -1 on failure
 */
int htm_record(struct pdbg_target *target, char *filename);

/**
 * @brief DEPRECATED, do not use
 *
 * Read memory using an ADU
 * @param[in] target adu target to use
 * @param[in] addr physical address to read
 * @param[out] output buffer to hold the results of the read
 * @param[in] size size of the output buffer
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to read size bytes starting at addr into
 * the output buffer.
 */
int adu_getmem(struct pdbg_target *target, uint64_t addr,
	       uint8_t *output, uint64_t size);

/**
 * @brief DEPRECATED, do not use
 *
 * Write memory using an ADU
 * @param[in] target adu target to use
 * @param[in] addr physical address to read
 * @param[in] input buffer containing the values to write
 * @param[in] size size of the input buffer
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to write size bytes from the input buffer
 * to memory starting at addr.
 */
int adu_putmem(struct pdbg_target *target, uint64_t addr,
	       uint8_t *input, uint64_t size);

/**
 * @brief DEPRECATED, do not use
 *
 * Read cache inhibited memory using an ADU
 * @param[in] target adu target to use
 * @param[in] addr physical address to read
 * @param[out] output buffer to hold the results of the read
 * @param[in] size size of the output buffer
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to read size bytes of cache inhibited
 * memory starting at addr into the output buffer.
 */
int adu_getmem_ci(struct pdbg_target *target, uint64_t addr,
		  uint8_t *output, uint64_t size);

/**
 * @brief DEPRECATED, do not use
 *
 * Write cache inhibited memory using an ADU
 * @param[in] target adu target to use
 * @param[in] addr physical address to read
 * @param[in] input buffer containing the values to write
 * @param[in] size size of the input buffer
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to write size bytes from the input buffer
 * to cache inhibited memory starting at addr.
 */
int adu_putmem_ci(struct pdbg_target *target, uint64_t addr,
		  uint8_t *input, uint64_t size);

/**
 * @brief DEPRECATED, do not use
 *
 * Read IO memory using an ADU
 * @param[in] adu_target adu target to use
 * @param[in] start_addr physical address to read
 * @param[out] output buffer to hold the results of the read
 * @param[in] size size of the output buffer
 * @param[in] blocksize specific size of each read request generated
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to read size bytes starting at IO memory
 * addr into the output buffer. Assumes memory is cache inhibited and
 * that read commands must match the given block size.
 */
int adu_getmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *output, uint64_t size, uint8_t blocksize);

/**
 * @brief DEPRECATED, do not use
 *
 * Write IO memory using an ADU
 * @param[in] adu_target adu target to use
 * @param[in] start_addr physical address to read
 * @param[in] input buffer to hold the results of the read
 * @param[in] size size of the output buffer
 * @param[in] block_size specific size of each read request generated
 * @returns 0 on success, -1 on failure
 *
 * Uses the given ADU target to read size bytes starting at IO memory
 * addr into the output buffer. Assumes memory is cache inhibited and
 * that read commands must match the given block size.
 */
int adu_putmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *input, uint64_t size, uint8_t block_size);

/**
 * @brief Convenience function for calling adu_getmem[_ci]()
 */
int __adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *ouput,
		 uint64_t size, bool ci);

/**
 * @brief Convenience function for calling adu_putmem[_ci]()
 */
int __adu_putmem(struct pdbg_target *target, uint64_t addr, uint8_t *input,
		 uint64_t size, bool ci);

/**
 * @brief Read memory using a mem class target (e.g. ADU)
 *
 * @param[in]  target the mem class target to operate on
 * @param[in]  addr physical address to read
 * @param[out] output buffer to hold the results of the read
 * @param[in]  size size of the output buffer
 * @param[in]  block_size hardware read size
 * @param[in]  ci use cache inhibited access
 *
 * @return 0 on success, -1 on failure
 *
 * Uses the given mem class target to read size bytes starting at addr into
 * the output buffer.  Cache inhibited memory can be accesssed by setting ci
 * to true.  For reading IO memory, read commands must match the given block
 * size.
 */
int mem_read(struct pdbg_target *target, uint64_t addr, uint8_t *output, uint64_t size, uint8_t block_size, bool ci);

/**
 * @brief Write memory using a mem class target (e.g. ADU)
 *
 * @param[in]  target the mem class target to operate on
 * @param[in]  addr physical address to write
 * @param[in]  input buffer containing the values to write
 * @param[in]  size size of the input buffer
 * @param[in]  block_size hardware write size
 * @param[in]  ci use cache inhibited access
 *
 * @return 0 on success, -1 on failure
 *
 * Uses the given mem class target to write size bytes from the input buffer
 * to memory starting at addr.  Cache inhibited memory can be written by
 * setting ci to true.  For writing IO memory, write commands must match given
 * block size.
 */
int mem_write(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size, uint8_t block_size, bool ci);

/**
 * @brief Read a register on an OPB
 * @param[in] target pdbg_target on the OPB to read
 * @param[in] addr address offset relative to target to read from
 * @param[out] data value of the register
 * @return 0 on success, -1 otherwise
 */
int opb_read(struct pdbg_target *target, uint32_t addr, uint32_t *data);

/**
 * @brief Write a register on an OPB
 * @param[in] target pdbg_target on the OPB to write
 * @param[in] addr address offset relative to target to write
 * @param[in] data value to write to the register
 * @return 0 on success, -1 otherwise
 */
int opb_write(struct pdbg_target *target, uint32_t addr, uint32_t data);

/**
 * @brief Execute IPL istep using SBE
 *
 * @param[in]  target pib target to operate on
 * @param[in]  major istep major number
 * @param[in]  minor istep minor number
 *
 * @return 0 on success, errno on failure
 */
int sbe_istep(struct pdbg_target *target, uint32_t major, uint32_t minor);

/**
 * @brief Get FFDC data if error is generated
 *
 * @param[in]  target pib target to operate on
 * @param[out] ffdc pointer to output buffer to store ffdc data
 * @param[out] ffdc_len the sizeof the ffdc data returned
 *
 * @return SBE error code, 0 if there is no ffdc data
 *
 * The ffdc data must be freed by caller.
 */
uint32_t sbe_ffdc_get(struct pdbg_target *target, const uint8_t **ffdc, uint32_t *ffdc_len);

/**
 * @brief Execute enter mpipl on the pib
 *
 * @param[in] target pib target to operate on
 *
 * @return 0 on success, -1 on failure
 */
int sbe_mpipl_enter(struct pdbg_target *target);

/**
 * @brief Execute continue mpipl on the pib
 *
 * @param[in] target pib target to operate on
 *
 * @return 0 on success, -1 on failure
 */
int sbe_mpipl_continue(struct pdbg_target *target);

/**
 * @brief Type for specifying a progress callback for long running
 * operations
 *
 * @see pdbg_set_progress_tick()
 * @see pdbg_progress_tick()
 */
typedef void (*pdbg_progress_tick_t)(uint64_t cur, uint64_t end);

/**
 * @brief Set a callback to be called periodically during long running
 * operations
 * @param[in] fn callback
 * @return void
 *
 * Certain applications may want to print/update a progress bar or do
 * some other work during long running operations such as reading
 * large amounts of memory. This function lets an application set a
 * callback to do this.
 */
void pdbg_set_progress_tick(pdbg_progress_tick_t fn);

/**
 * @brief Update the progress of an operation
 * @param[in] cur current count of progress
 * @param[in] end final progress count
 *
 * Calls the callback registered via pdbg_set_progress_tick(). The
 * current progress count should represent how close the operation is
 * to completing relative to the final progress count. Callbacks
 * should be able use these values to calculate the percentage
 * complete using cur/end*100.
 */
void pdbg_progress_tick(uint64_t cur, uint64_t end);

#define PDBG_ERROR	0
#define PDBG_WARNING	1
#define PDBG_NOTICE	2
#define PDBG_INFO	3
#define PDBG_DEBUG	4

/**
 * @brief Type for specifying a callback for logging trace information
 * @param[in] loglevel level to log this message at
 * @param[in] printf style format string containing the message to log
 * @param[in] ap arguments for the printf string
 */
typedef void (*pdbg_log_func_t)(int loglevel, const char *fmt, va_list ap);

/**
 * @brief Set the logging callback function
 * @param[in] fn pointer to the logging function
 */
void pdbg_set_logfunc(pdbg_log_func_t fn);

/**
 * @brief Set the log level
 * @param[in] loglevel
 *
 * The logging callback will only get called for levels less than or
 * equal to the current loglevel.
 */
void pdbg_set_loglevel(int loglevel);

/**
 * @brief Log a message via the log callback
 * @param[in] loglevel level to log the message at
 * @param[in] fmt printf style format string containing the message
 */
void pdbg_log(int loglevel, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
