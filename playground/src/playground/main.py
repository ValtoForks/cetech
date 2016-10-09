import sys

from PyQt5.QtWidgets import QApplication
from playground.core.projectmanagerdialog import ProjectManagerDialog

from playground.core.mainwindow import MainWindow


def main():
    app = QApplication(sys.argv)
    app.setOrganizationName('cyberegoorg')
    app.setApplicationName('playground')

    pm = ProjectManagerDialog()
    ret = pm.exec()

    if ret == pm.Accepted:
        name, directory = pm.open_project_name, pm.open_project_dir

        mw = MainWindow()

        mw.show()
        mw.focusWidget()
        mw.open_project(name, directory)

        try:
            ret = app.exec_()
        finally:
            mw.project.killall()

        sys.exit(ret)

    else:
        sys.exit(0)
