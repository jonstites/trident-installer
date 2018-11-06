//============================
//  Project Trident source code
//  Copyright (c) 2018, Ken Moore
//  Available under the 2-clause BSD license
//  See the LICENSE file for full details
//============================
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QJsonArray>
#include <QTimeZone>
#include <QDebug>
#include <QtConcurrent>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#define INSTALLLOG "/tmp/.pc-sysinstall/pc-sysinstall.log"
#define INSTALLCONF "/tmp/trident-sysinstall.conf"
#define REFIND_ZIP "/Trident_refind.zip"

struct userdata{
	QString name, comment, pass, shell, home;
	QStringList groups;
	bool autologin;
};

struct partitiondata{
	QString install_type; //install_type: "ZFS", "SWAP", "UFS" (*.eli for GELI encrypted version)
	QString encrypt_pass;
	QStringList create_partitions; //datasets if using ZFS, partitions if using something else
	double sizeMB; //size in MB; set to <=0 for automatic
	QString extrasetup;
};

struct diskdata{
	QString name, install_partition; //name: ada0, install_partition: "all", "free", "s1", "s2" ...
	QString mirror_disk; //Used for old-style gmirror setup
	bool installBootManager;
	QList<partitiondata> partitions;
};



class Backend : public QObject{
	Q_OBJECT
private:
	QJsonObject settings, keyboardInfo, pciconf;
	QList<userdata> USERS;
	QList<diskdata> DISKS;
	QProcess *PROC;

	QString generateInstallConfig(bool internal = true); //turns JSON settings into a config file for pc-sysinstall
	QString getLocalDistPath(); //Determine the dir path to the dist files for install

	//Functions for filling the private variables as needed (always run them before using)
	void checkKeyboardInfo();
	void checkPciConf();

	//Recursive generation functions
	void GeneratePackageItem(QJsonObject json, QTreeWidget *tree, QString name, QTreeWidgetItem *parent = 0);

	// General-use functions
	QString bytesToHuman(QString bytes);

public:
	Backend(QObject *parent);
	~Backend();

	static QString runCommand(bool &success, QString command, QStringList arguments = QStringList(), QString workdir = "", QStringList env = QStringList());
	static QString mbToHuman(double);
	static QString readFile(QString path);
	static bool writeFile(QString path, QString contents);

	// Information
	QString generateSummary(){ return generateInstallConfig(false); }
	bool isLaptop();
	QString system_information();
	QString pci_info();
	bool isUEFI();
	QString isodate();
	// Post-install information
	QString fullInstallLog(){ return readFile(INSTALLLOG); }
	QString fullInstallConf(){ return readFile(INSTALLCONF); }

	//Localization
	QString lang();
	void setLang(QString);
	QStringList availableLanguages();
	//Keyboard Settings
	QStringList keyboard(); //layout, model, variant
	void setKeyboard(QStringList, bool changenow = true); //layout, model, variant
	QJsonObject availableKeyboardModels();
	QJsonObject availableKeyboardLayouts();
	//Time Settings
	QString timezone();
	void setTimezone(QString);
	QDateTime localDateTime();
	void setDateTime(QDateTime); //alternate to setTimezone() - sets both timezone and offset from system time
	QStringList availableTimezones();
	bool useNTP();
	void setUseNTP(bool);
	//Hostname
	QString hostname();
	void setHostname(QString);

	//System Users
	QString rootPass();
	void setRootPass(QString);
	QList<userdata> users(){ return USERS; }
	void addUser(userdata); //will overwrite existing user with the same "name"
	void removeUser(QString name);
	void clearUsers(); //remove all users

	//Disk Partitioning
	// - Information functions
	QJsonObject availableDisks(bool fromcache = true);
	QString diskInfoObjectToString(QJsonObject obj);
	QString diskInfoObjectToShortString(QJsonObject obj);
	bool checkValidSize(QJsonObject obj, bool installdrive = true, bool freespaceonly = false);
	// - ZFS Install to BE option
	QStringList availableZPools();
	bool installToBE(); //will report true if a valid ZFS pool was designated
	QString zpoolName(); //will return the designated ZFS pool name
	void setInstallToBE(QString pool); //set to an empty string to disable installing to a BE
	// Custom ZPool name
	QString customPoolName();
	void setCustomPoolName(QString name);
	// - Individual disk setup
	QList<diskdata> disks(){ return DISKS; }
	void addDisk(diskdata); //will overwrite existing disk with the same name
	void removeDisk(QString name);
	void clearDisks();
	QStringList generateDefaultZFSPartitions();
	// - 4K Disk alignment
	bool use_4k_alignment(); //enabled by default
	void set4k_alignment(bool set);

	// Boot Manager Installations
	bool BM_refindAvailable();
	bool install_refind();

	//Packages
	QString dist_package_dir();
	QJsonObject package_info(QString pkgname);
	QStringList availableShells(QTreeWidget *pkgtree);
	QStringList defaultUserShell(); //list of defaults in order of importance
	void populatePackageTreeWidget(QTreeWidget *tree);
	void setInstallPackages(QStringList);
	void setInstallPackages(QTreeWidget *tree); //overload - probe the tree widget for the list

private slots:
	//Installation
	void read_install_output();
	void install_finished(int, QProcess::ExitStatus);

public slots:
	void startInstallation();

signals:
	void keyboardInfoAvailable();
	void install_update(QString);
	void install_started();
	void install_finished(bool);
};
