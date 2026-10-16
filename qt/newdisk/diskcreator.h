/* === creating new disk images in the Qt port ===
 * File name first, then the details for its format, as the Windows port's
 * dialog_newdisk_ex (windows/dialog/d_disk.cpp) does. Hard disk images are
 * written on a worker thread with progress and cancel.
 */

#ifndef NP2_QT_DISKCREATOR_H
#define NP2_QT_DISKCREATOR_H

class QWidget;

namespace DiskCreator {

void newFdImage(QWidget *parent);
void newHdImage(QWidget *parent);

} // namespace DiskCreator

#endif /* NP2_QT_DISKCREATOR_H */
