#ifndef DEEP_CLONE_INSTRUMENT_QT_COMMAND_HPP
#define DEEP_CLONE_INSTRUMENT_QT_COMMAND_HPP

#include <QUndoCommand>
#include <QListWidget>
#include <memory>
#include "gui/instrument_editor/instrument_form_manager.hpp"
#include "misc.hpp"

class DeepCloneInstrumentQtCommand : public QUndoCommand
{
public:
	DeepCloneInstrumentQtCommand(QListWidget *list, int num, int refNum,
							 std::weak_ptr<InstrumentFormManager> formMan,
							 QUndoCommand* parent = nullptr);

	void undo() Q_DECL_OVERRIDE;
	void redo() Q_DECL_OVERRIDE;
	int id() const Q_DECL_OVERRIDE;

private:
	QListWidget* list_;
	int cloneNum_, refNum_;
	std::weak_ptr<InstrumentFormManager> formMan_;
	SoundSource source_;
};

#endif // DEEP_CLONE_INSTRUMENT_QT_COMMAND_HPP
